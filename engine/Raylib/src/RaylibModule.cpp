#include "Scrapper/Raylib/RaylibModule.h"
#include "Scrapper/Raylib/Cameras.h"
#include "Scrapper/Raylib/RectLayout.h"
#include "Scrapper/Raylib/RenderSettings.h"
#include "Scrapper/Raylib/Renderers.h"
#include "Scrapper/Core/Lifecycle.h"
#include "Scrapper/Core/Transform.h"
#include "Scrapper/Core/World.h"

#include <rlgl.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>


namespace Scrapper {
namespace {
constexpr std::size_t PrimitiveCount = static_cast<std::size_t>(PrimitiveMesh::Count);

// Lambert diffuse from one directional light + a sky/ground hemisphere ambient.
constexpr const char* LitVertexShader = R"(#version 330
in vec3 vertexPosition;
in vec3 vertexNormal;
uniform mat4 mvp;
uniform mat4 matNormal;
out vec3 fragNormal;
void main() {
    fragNormal = (matNormal * vec4(vertexNormal, 0.0)).xyz;
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
)";

constexpr const char* LitFragmentShader = R"(#version 330
in vec3 fragNormal;
uniform vec4 colDiffuse;
uniform vec3 lightDirection;
out vec4 finalColor;
void main() {
    vec3 normal = normalize(fragNormal);
    float diffuse = max(dot(normal, -lightDirection), 0.0);
    float hemisphere = 0.5 + 0.5 * normal.y;
    vec3 color = colDiffuse.rgb * (0.22 + 0.18 * hemisphere + 0.7 * diffuse);
    finalColor = vec4(color, colDiffuse.a);
}
)";

// A ModelAsset uploaded to the GPU. Holding the asset keeps the cache key alive.
struct GpuModel {
    std::shared_ptr<const ModelAsset> source;
    std::vector<Mesh> meshes;
    std::vector<Mesh> colliders;
};

// GPU-side state owned by the module.
struct RaylibBackend {
    RenderTexture2D target = {};
    Font font = {};
    std::array<Mesh, PrimitiveCount> meshes = {};
    std::array<Matrix, PrimitiveCount> pivots = {};
    Material litMaterial = {};
    int lightDirectionLoc = -1;
    std::unordered_map<const ModelAsset*, GpuModel> models;
};

// Triangles are unrolled (no index buffer): raylib indexes with 16 bits, and these meshes are small.
Mesh UploadMesh(const std::vector<Vector3>& positions, const std::vector<Vector3>& normals,
                const std::vector<std::uint32_t>& indices) {
    Mesh mesh = {};
    mesh.vertexCount = static_cast<int>(indices.size());
    mesh.triangleCount = mesh.vertexCount / 3;
    mesh.vertices = static_cast<float*>(MemAlloc(static_cast<unsigned int>(indices.size() * 3 * sizeof(float))));
    mesh.normals = static_cast<float*>(MemAlloc(static_cast<unsigned int>(indices.size() * 3 * sizeof(float))));

    for (std::size_t t = 0; t + 2 < indices.size(); t += 3) {
        const Vector3 a = positions[indices[t]];
        const Vector3 b = positions[indices[t + 1]];
        const Vector3 c = positions[indices[t + 2]];
        const Vector3 face = Vector3Normalize(Vector3CrossProduct(Vector3Subtract(b, a), Vector3Subtract(c, a)));
        for (std::size_t k = 0; k < 3; ++k) {
            const std::uint32_t index = indices[t + k];
            const Vector3 p = positions[index];
            const Vector3 n = normals.empty() ? face : normals[index];
            std::memcpy(mesh.vertices + (t + k) * 3, &p, sizeof(Vector3));
            std::memcpy(mesh.normals + (t + k) * 3, &n, sizeof(Vector3));
        }
    }

    ::UploadMesh(&mesh, false);
    return mesh;
}

GpuModel& Upload(RaylibBackend& backend, const std::shared_ptr<const ModelAsset>& model) {
    auto it = backend.models.find(model.get());
    if (it != backend.models.end()) return it->second;

    GpuModel gpu;
    gpu.source = model;
    for (const MeshPart& part : model->meshes) gpu.meshes.push_back(UploadMesh(part.positions, part.normals, part.indices));
    for (const ConvexPart& part : model->colliders) gpu.colliders.push_back(UploadMesh(part.positions, {}, part.indices));
    return backend.models.emplace(model.get(), std::move(gpu)).first->second;
}

struct Command2D {
    enum class Kind { Rect, Sprite, Text } kind = Kind::Rect;
    Vector2 position = { 0.0f, 0.0f };
    float rotation = 0.0f; // degrees
    Vector2 scale = { 1.0f, 1.0f };
    Vector2 size = { 0.0f, 0.0f };
    Vector2 pivot = { 0.5f, 0.5f };
    Color color = WHITE;
    int layer = 0;
    Texture2D texture = {};
    Rectangle source = {};
    const std::string* text = nullptr;
    int fontSize = 20;
};

float RotationAroundZ(Quaternion q) {
    return 2.0f * std::atan2(q.z, q.w) * RAD2DEG;
}

Command2D Base(const WorldTransform& world, int layer) {
    Command2D cmd;
    cmd.position = { world.position.x, world.position.y };
    cmd.rotation = RotationAroundZ(world.rotation);
    cmd.scale = { world.scale.x, world.scale.y };
    cmd.layer = layer;
    return cmd;
}

void Gather2D(entt::registry& registry, std::vector<Command2D>& worldCommands, std::vector<Command2D>& uiCommands) {
    const auto queue = [&](entt::entity entity) -> std::vector<Command2D>& {
        return registry.all_of<RectLayout>(entity) ? uiCommands : worldCommands;
    };

    for (auto [entity, rect, world] : registry.view<RectRenderer, WorldTransform>(entt::exclude<Disabled>).each()) {
        Command2D cmd = Base(world, rect.layer);
        cmd.kind = Command2D::Kind::Rect;
        cmd.size = rect.size;
        cmd.pivot = rect.pivot;
        cmd.color = rect.color;
        queue(entity).push_back(cmd);
    }
    for (auto [entity, sprite, world] : registry.view<SpriteRenderer, WorldTransform>(entt::exclude<Disabled>).each()) {
        if (sprite.texture.id == 0) continue;
        Command2D cmd = Base(world, sprite.layer);
        cmd.kind = Command2D::Kind::Sprite;
        cmd.size = sprite.size;
        cmd.pivot = sprite.pivot;
        cmd.color = sprite.tint;
        cmd.texture = sprite.texture;
        cmd.source = sprite.source;
        queue(entity).push_back(cmd);
    }
    for (auto [entity, text, world] : registry.view<TextRenderer, WorldTransform>(entt::exclude<Disabled>).each()) {
        Command2D cmd = Base(world, text.layer);
        cmd.kind = Command2D::Kind::Text;
        cmd.color = text.color;
        cmd.text = &text.text;
        cmd.fontSize = static_cast<int>(static_cast<float>(text.fontSize) * (world.scale.x + world.scale.y) * 0.5f);
        queue(entity).push_back(cmd);
    }

    const auto byLayer = [](const Command2D& a, const Command2D& b) { return a.layer < b.layer; };
    std::stable_sort(worldCommands.begin(), worldCommands.end(), byLayer);
    std::stable_sort(uiCommands.begin(), uiCommands.end(), byLayer);
}

void Execute2D(const RaylibBackend& backend, const std::vector<Command2D>& commands) {
    for (const Command2D& cmd : commands) {
        switch (cmd.kind) {
            case Command2D::Kind::Rect: {
                const float width = cmd.size.x * cmd.scale.x;
                const float height = cmd.size.y * cmd.scale.y;
                DrawRectanglePro({ cmd.position.x, cmd.position.y, width, height },
                                 { width * cmd.pivot.x, height * cmd.pivot.y }, cmd.rotation, cmd.color);
                break;
            }
            case Command2D::Kind::Sprite: {
                const float width = cmd.size.x * cmd.scale.x;
                const float height = cmd.size.y * cmd.scale.y;
                DrawTexturePro(cmd.texture, cmd.source, { cmd.position.x, cmd.position.y, width, height },
                               { width * cmd.pivot.x, height * cmd.pivot.y }, cmd.rotation, cmd.color);
                break;
            }
            case Command2D::Kind::Text: {
                if (backend.font.texture.id > 0) {
                    DrawTextEx(backend.font, cmd.text->c_str(), cmd.position, static_cast<float>(cmd.fontSize), 0.0f, cmd.color);
                } else {
                    DrawText(cmd.text->c_str(), static_cast<int>(cmd.position.x), static_cast<int>(cmd.position.y),
                             cmd.fontSize, cmd.color);
                }
                break;
            }
        }
    }
}

void Draw3D(entt::registry& registry, RaylibBackend& backend, const RenderSettings& settings) {
    if (backend.litMaterial.shader.id == 0) return;

    const Vector3 light = Vector3Normalize(settings.lightDirection);
    SetShaderValue(backend.litMaterial.shader, backend.lightDirectionLoc, &light, SHADER_UNIFORM_VEC3);

    for (auto [entity, mesh, world] : registry.view<MeshRenderer, WorldTransform>(entt::exclude<Disabled>).each()) {
        const std::size_t index = static_cast<std::size_t>(mesh.mesh);
        backend.litMaterial.maps[MATERIAL_MAP_DIFFUSE].color = mesh.color;
        const Matrix model = MatrixMultiply(MatrixScale(mesh.size.x, mesh.size.y, mesh.size.z), world.matrix);
        DrawMesh(backend.meshes[index], backend.litMaterial, MatrixMultiply(backend.pivots[index], model));
    }

    for (auto [entity, renderer, world] : registry.view<ModelRenderer, WorldTransform>(entt::exclude<Disabled>).each()) {
        if (renderer.model == nullptr) continue;
        const GpuModel& gpu = Upload(backend, renderer.model);

        for (std::size_t i = 0; i < gpu.meshes.size(); ++i) {
            backend.litMaterial.maps[MATERIAL_MAP_DIFFUSE].color = ColorTint(renderer.model->meshes[i].color, renderer.tint);
            DrawMesh(gpu.meshes[i], backend.litMaterial, world.matrix);
        }

        if (renderer.colliders == ColliderDisplay::Hidden) continue;
        backend.litMaterial.maps[MATERIAL_MAP_DIFFUSE].color = renderer.colliderColor;
        if (renderer.colliders == ColliderDisplay::Wireframe) rlEnableWireMode();
        for (const Mesh& mesh : gpu.colliders) DrawMesh(mesh, backend.litMaterial, world.matrix);
        if (renderer.colliders == ColliderDisplay::Wireframe) rlDisableWireMode();
    }
}

Rectangle Viewport(const RenderTexture2D& target) {
    const float width = static_cast<float>(target.texture.width);
    const float height = static_cast<float>(target.texture.height);
    const float scale = std::min(static_cast<float>(GetScreenWidth()) / width, static_cast<float>(GetScreenHeight()) / height);
    return { (static_cast<float>(GetScreenWidth()) - width * scale) * 0.5f,
             (static_cast<float>(GetScreenHeight()) - height * scale) * 0.5f, width * scale, height * scale };
}

void Render(World& world, float) {
    entt::registry& registry = world.Registry();
    RaylibBackend& backend = world.Resource<RaylibBackend>();
    const RenderSettings& settings = world.Resource<RenderSettings>();

    std::vector<Command2D> world2D;
    std::vector<Command2D> ui;
    Gather2D(registry, world2D, ui);

    struct CameraRef { int order; entt::entity entity; bool is3D; };
    std::vector<CameraRef> cameras;
    for (entt::entity entity : registry.view<CameraView3D, WorldTransform>(entt::exclude<Disabled>)) {
        cameras.push_back({ registry.get<CameraView3D>(entity).order, entity, true });
    }
    for (entt::entity entity : registry.view<CameraView2D, WorldTransform>(entt::exclude<Disabled>)) {
        cameras.push_back({ registry.get<CameraView2D>(entity).order, entity, false });
    }
    std::stable_sort(cameras.begin(), cameras.end(), [](const CameraRef& a, const CameraRef& b) { return a.order < b.order; });

    BeginTextureMode(backend.target);
    ClearBackground(settings.clearColor);

    bool drewWorld2D = false;
    for (const CameraRef& ref : cameras) {
        const WorldTransform& transform = registry.get<WorldTransform>(ref.entity);
        if (ref.is3D) {
            const CameraView3D& view = registry.get<CameraView3D>(ref.entity);
            Camera3D camera = {};
            camera.position = transform.position;
            camera.target = Vector3Add(transform.position, Transforms::Forward(transform.rotation));
            camera.up = Transforms::Up(transform.rotation);
            camera.fovy = view.fovy;
            camera.projection = view.projection;
            BeginMode3D(camera);
            Draw3D(registry, backend, settings);
            EndMode3D();
        } else {
            const CameraView2D& view = registry.get<CameraView2D>(ref.entity);
            BeginMode2D({ view.offset, { transform.position.x, transform.position.y }, RotationAroundZ(transform.rotation), view.zoom });
            Execute2D(backend, world2D);
            EndMode2D();
            drewWorld2D = true;
        }
    }
    if (!drewWorld2D) {
        Execute2D(backend, world2D);
    }
    Execute2D(backend, ui);
    EndTextureMode();

    BeginDrawing();
    ClearBackground(BLACK);
    const Rectangle source = { 0.0f, 0.0f, static_cast<float>(backend.target.texture.width),
                               -static_cast<float>(backend.target.texture.height) };
    DrawTexturePro(backend.target.texture, source, Viewport(backend.target), { 0.0f, 0.0f }, 0.0f, WHITE);
    EndDrawing(); // also polls input for the next frame

    if (WindowShouldClose()) {
        world.RequestQuit();
    }
}

void Layout(World& world, float) {
    entt::registry& registry = world.Registry();
    const Vector2 target = world.Resource<RenderSettings>().targetResolution;

    for (auto [entity, layout, transform] : registry.view<RectLayout, LocalTransform>().each()) {
        Vector2 offset = layout.anchoredPosition;
        const entt::entity parent = Transforms::GetParent(registry, entity);
        if (const RectLayout* parentRect = parent != entt::null ? registry.try_get<RectLayout>(parent) : nullptr) {
            offset.x += parentRect->size.x * (layout.anchor.x - parentRect->pivot.x);
            offset.y += parentRect->size.y * (layout.anchor.y - parentRect->pivot.y);
        } else {
            offset.x += target.x * layout.anchor.x;
            offset.y += target.y * layout.anchor.y;
        }
        transform.position = { offset.x, offset.y, 0.0f };
    }
}

Font LoadDefaultFont(const std::string& path) {
    if (!FileExists(path.c_str())) return {};

    std::vector<int> codepoints;
    for (int c = 32; c < 127; ++c) codepoints.push_back(c);         // ASCII
    for (int c = 0xAC00; c <= 0xD7A3; ++c) codepoints.push_back(c); // Hangul syllables
    return LoadFontEx(path.c_str(), 32, codepoints.data(), static_cast<int>(codepoints.size()));
}
}

void RaylibModule::Install(World& world) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(config.width, config.height, config.title.c_str());
    SetTargetFPS(config.targetFps);

    RaylibBackend& backend = world.AddResource<RaylibBackend>();
    backend.target = LoadRenderTexture(config.targetWidth, config.targetHeight);
    backend.font = LoadDefaultFont(config.fontPath);

    backend.meshes[static_cast<std::size_t>(PrimitiveMesh::Cube)] = GenMeshCube(1.0f, 1.0f, 1.0f);
    backend.meshes[static_cast<std::size_t>(PrimitiveMesh::Sphere)] = GenMeshSphere(0.5f, 16, 24);
    backend.meshes[static_cast<std::size_t>(PrimitiveMesh::Cylinder)] = GenMeshCylinder(0.5f, 1.0f, 24);
    backend.meshes[static_cast<std::size_t>(PrimitiveMesh::Plane)] = GenMeshPlane(1.0f, 1.0f, 1, 1);
    backend.pivots.fill(MatrixIdentity());
    // raylib builds cylinders from y = 0 upward; shift them so every primitive is centered.
    backend.pivots[static_cast<std::size_t>(PrimitiveMesh::Cylinder)] = MatrixTranslate(0.0f, -0.5f, 0.0f);

    backend.litMaterial = LoadMaterialDefault();
    backend.litMaterial.shader = LoadShaderFromMemory(LitVertexShader, LitFragmentShader);
    backend.lightDirectionLoc = GetShaderLocation(backend.litMaterial.shader, "lightDirection");

    RenderSettings& settings = world.AddResource<RenderSettings>();
    settings.targetResolution = { static_cast<float>(config.targetWidth), static_cast<float>(config.targetHeight) };

    Pipeline& pipeline = world.GetPipeline();
    pipeline.Add(RaylibSystem::Layout, &Layout, Placement::InPhase(Phase::PreSimulate).Before(CoreSystem::PropagatePreSimulate));
    pipeline.Add(RaylibSystem::Render, &Render, Placement::InPhase(Phase::Render).After(CoreSystem::PropagateRender));
}

void RaylibModule::Uninstall(World& world) {
    Pipeline& pipeline = world.GetPipeline();
    pipeline.Remove(RaylibSystem::Layout);
    pipeline.Remove(RaylibSystem::Render);

    RaylibBackend& backend = world.Resource<RaylibBackend>();
    for (Mesh& mesh : backend.meshes) {
        if (mesh.vaoId != 0) UnloadMesh(mesh);
    }
    for (auto& [key, gpu] : backend.models) {
        for (Mesh& mesh : gpu.meshes) UnloadMesh(mesh);
        for (Mesh& mesh : gpu.colliders) UnloadMesh(mesh);
    }
    backend.models.clear();
    if (backend.litMaterial.maps != nullptr) UnloadMaterial(backend.litMaterial); // also unloads its shader
    if (backend.font.texture.id > 0) UnloadFont(backend.font);
    if (backend.target.id != 0) UnloadRenderTexture(backend.target);

    world.RemoveResource<RaylibBackend>();
    world.RemoveResource<RenderSettings>();
    CloseWindow();
}

}
