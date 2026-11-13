#include "Rendering/RenderSystem.h"
#include "Rendering/CameraComponent/CameraComponent.h"
#include "Rendering/RendererComponent/Renderer.h"
#include <algorithm>
#include <raymath.h>


namespace ScrapperEngine {
void RenderSystem::ConfigureTargetResolution(int width, int height) {
    if (width <= 0 || height <= 0) {
        return;
    }

    targetWidth = width;
    targetHeight = height;
}

void RenderSystem::Initialize() {
    if (initialized) {
        return;
    }

    RecreateTargetTexture();
    initialized = targetTexture.id != 0;

    if (initialized) {
        Load3DResources();
    }
}

void RenderSystem::Shutdown() {
    Clear();
    Unload3DResources();

    if (targetTexture.id != 0) {
        UnloadRenderTexture(targetTexture);
        targetTexture = {};
    }

    initialized = false;
}

void RenderSystem::RenderFrame() {
    if (!initialized || targetTexture.id == 0) {
        BeginDrawing();
        ClearBackground(BLACK);
        RenderAll();
        EndDrawing();
        return;
    }

    BeginTextureMode(targetTexture);
    ClearBackground(clearColor);
    RenderAll();
    EndTextureMode();

    BeginDrawing();
    ClearBackground(BLACK);

    Rectangle source = {
        0.0f,
        0.0f,
        static_cast<float>(targetTexture.texture.width),
        -static_cast<float>(targetTexture.texture.height)
    };
    Rectangle destination = GetViewportRect();
    Vector2 origin = { 0.0f, 0.0f };

    DrawTexturePro(targetTexture.texture, source, destination, origin, 0.0f, WHITE);

    EndDrawing();
}

void RenderSystem::SetTargetResolution(int width, int height) {
    if (width <= 0 || height <= 0 || (width == targetWidth && height == targetHeight)) {
        return;
    }

    targetWidth = width;
    targetHeight = height;

    if (initialized && IsWindowReady()) {
        RecreateTargetTexture();
    }
}

Vector2 RenderSystem::GetTargetResolution() const {
    return {
        static_cast<float>(targetWidth),
        static_cast<float>(targetHeight)
    };
}

Rectangle RenderSystem::GetViewportRect() const {
    if (targetWidth <= 0 || targetHeight <= 0) {
        return { 0.0f, 0.0f, 0.0f, 0.0f };
    }

    const float scale = GetRenderScale();
    const float width = static_cast<float>(targetWidth) * scale;
    const float height = static_cast<float>(targetHeight) * scale;

    return {
        (static_cast<float>(GetScreenWidth()) - width) * 0.5f,
        (static_cast<float>(GetScreenHeight()) - height) * 0.5f,
        width,
        height
    };
}

float RenderSystem::GetRenderScale() const {
    if (targetWidth <= 0 || targetHeight <= 0) {
        return 1.0f;
    }

    return std::min(
        static_cast<float>(GetScreenWidth()) / static_cast<float>(targetWidth),
        static_cast<float>(GetScreenHeight()) / static_cast<float>(targetHeight)
    );
}

Vector2 RenderSystem::ScreenToTarget(Vector2 screenPosition) const {
    const Rectangle viewport = GetViewportRect();
    const float scale = GetRenderScale();

    if (scale <= 0.0f) {
        return { 0.0f, 0.0f };
    }

    return {
        (screenPosition.x - viewport.x) / scale,
        (screenPosition.y - viewport.y) / scale
    };
}

void RenderSystem::RegisterRenderer(Renderer* r) {
    if (r == nullptr || r->owner == nullptr) {
        return;
    }

    UnregisterRenderer(r);

    const RenderBucket bucket = r->ResolveBucket();
    std::vector<Renderer*>* renderers = BucketOf(bucket);
    if (renderers == nullptr) {
        return;
    }

    r->renderBucket = bucket;
    r->renderIndex = static_cast<int>(renderers->size());
    renderers->push_back(r);
}

void RenderSystem::UnregisterRenderer(Renderer* r) {
    if (r == nullptr) {
        return;
    }

    std::vector<Renderer*>* renderers = BucketOf(r->renderBucket);
    if (renderers == nullptr) {
        return;
    }

    // Swap-remove keeps every bucket dense.
    const int index = r->renderIndex;
    Renderer* moved = renderers->back();
    (*renderers)[static_cast<std::size_t>(index)] = moved;
    moved->renderIndex = index;
    renderers->pop_back();

    r->renderIndex = -1;
    r->renderBucket = RenderBucket::None;
}

std::vector<Renderer*>* RenderSystem::BucketOf(RenderBucket bucket) {
    switch (bucket) {
        case RenderBucket::World2D: return &world2DRenderers;
        case RenderBucket::World3D: return &world3DRenderers;
        case RenderBucket::UI:      return &uiRenderers;
        case RenderBucket::None:    break;
    }
    return nullptr;
}

void RenderSystem::RegisterCamera(CameraComponent* camera) {
    if (camera && std::find(registeredCameras.begin(), registeredCameras.end(), camera) == registeredCameras.end()) {
        registeredCameras.push_back(camera);
    }
}

void RenderSystem::UnregisterCamera(CameraComponent* camera) {
    auto it = std::find(registeredCameras.begin(), registeredCameras.end(), camera);
    if (it != registeredCameras.end()) {
        registeredCameras.erase(it);
    }
}

void RenderSystem::RenderAll() {
    // 1. Query active renderers from each dense renderer bucket.
    Clear();
    CollectCommands(world3DRenderers, nullptr);
    CollectCommands(world2DRenderers, &world2DCommands);
    CollectCommands(uiRenderers, &uiCommands);

    // 2. Sort 2D commands by layer ascending (3D relies on the depth buffer)
    const auto byLayer = [](const RenderCommand& a, const RenderCommand& b) {
        return a.layer < b.layer;
    };
    std::sort(world2DCommands.begin(), world2DCommands.end(), byLayer);
    std::sort(uiCommands.begin(), uiCommands.end(), byLayer);

    // 3. Render world commands through registered cameras in order.
    std::sort(registeredCameras.begin(), registeredCameras.end(), [](const CameraComponent* a, const CameraComponent* b) {
        return a->order < b->order;
    });

    bool renderedWorld2D = false;
    for (CameraComponent* camera : registeredCameras) {
        if (!camera->IsEnabled() || !camera->Begin()) {
            continue;
        }

        if (camera->Space() == RenderBucket::World3D) {
            Execute3DCommands();
        } else {
            ExecuteCommands(world2DCommands);
            renderedWorld2D = true;
        }
        camera->End();
    }

    if (!renderedWorld2D) {
        ExecuteCommands(world2DCommands);
    }

    // 4. Render UI commands (always screen space, on top of everything)
    ExecuteCommands(uiCommands);

    Clear();
}

void RenderSystem::Submit(const RenderCommand& cmd) {
    if (activeCommandQueue != nullptr) {
        activeCommandQueue->push_back(cmd);
    }
}

void RenderSystem::Submit(const RenderCommand3D& cmd) {
    world3DCommands.push_back(cmd);
}

void RenderSystem::CollectCommands(const std::vector<Renderer*>& renderers, std::vector<RenderCommand>* commands) {
    activeCommandQueue = commands;

    for (Renderer* r : renderers) {
        if (r && r->IsEnabled()) {
            r->Draw();
        }
    }

    activeCommandQueue = nullptr;
}

void RenderSystem::ExecuteCommands(const std::vector<RenderCommand>& cmds) {
    for (const auto& cmd : cmds) {
        switch (cmd.type) {
            case RenderType::RECTANGLE: {
                float finalWidth = cmd.size.x * cmd.scale.x;
                float finalHeight = cmd.size.y * cmd.scale.y;
                Rectangle rect = { cmd.position.x, cmd.position.y, finalWidth, finalHeight };
                Vector2 origin = { finalWidth * cmd.pivot.x, finalHeight * cmd.pivot.y }; // Pivot-based anchor
                DrawRectanglePro(rect, origin, cmd.rotation, cmd.color);
                break;
            }
            case RenderType::TEXTURE: {
                if (cmd.texture.id == 0) {
                    break;
                }

                float finalWidth = cmd.size.x * cmd.scale.x;
                float finalHeight = cmd.size.y * cmd.scale.y;
                Rectangle destRect = { cmd.position.x, cmd.position.y, finalWidth, finalHeight };
                Vector2 origin = { finalWidth * cmd.pivot.x, finalHeight * cmd.pivot.y };
                DrawTexturePro(cmd.texture, cmd.sourceRect, destRect, origin, cmd.rotation, cmd.color);
                break;
            }
            case RenderType::TEXT: {
                if (defaultFont.texture.id > 0) {
                    Vector2 pos = { cmd.position.x, cmd.position.y };
                    // defaultFont가 로드되어 있으면 DrawTextEx로 선명한 한글을 출력합니다.
                    DrawTextEx(defaultFont, cmd.text.c_str(), pos, static_cast<float>(cmd.fontSize), 0.0f, cmd.color);
                } else {
                    DrawText(cmd.text.c_str(), (int)cmd.position.x, (int)cmd.position.y, cmd.fontSize, cmd.color);
                }
                break;
            }
            case RenderType::LINE: {
                DrawLineEx(cmd.position, cmd.endPosition, cmd.thickness, cmd.color);
                break;
            }
        }
    }
}

void RenderSystem::Execute3DCommands() {
    if (litMaterial.shader.id == 0) {
        return;
    }

    SetShaderValue(litMaterial.shader, lightDirectionLoc, &lightDirection, SHADER_UNIFORM_VEC3);

    for (const RenderCommand3D& cmd : world3DCommands) {
        const std::size_t index = static_cast<std::size_t>(cmd.mesh);
        litMaterial.maps[MATERIAL_MAP_DIFFUSE].color = cmd.color;
        DrawMesh(primitiveMeshes[index], litMaterial, MatrixMultiply(primitivePivots[index], cmd.transform));
    }
}

void RenderSystem::SetLightDirection(Vector3 direction) {
    lightDirection = Vector3Normalize(direction);
}

namespace {
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
}

void RenderSystem::Load3DResources() {
    primitiveMeshes[static_cast<std::size_t>(PrimitiveMesh::Cube)] = GenMeshCube(1.0f, 1.0f, 1.0f);
    primitiveMeshes[static_cast<std::size_t>(PrimitiveMesh::Sphere)] = GenMeshSphere(0.5f, 16, 24);
    primitiveMeshes[static_cast<std::size_t>(PrimitiveMesh::Cylinder)] = GenMeshCylinder(0.5f, 1.0f, 24);
    primitiveMeshes[static_cast<std::size_t>(PrimitiveMesh::Plane)] = GenMeshPlane(1.0f, 1.0f, 1, 1);

    // raylib builds cylinders from y = 0 upward; shift them so every primitive is centered.
    primitivePivots.fill(MatrixIdentity());
    primitivePivots[static_cast<std::size_t>(PrimitiveMesh::Cylinder)] = MatrixTranslate(0.0f, -0.5f, 0.0f);

    litMaterial = LoadMaterialDefault();
    litMaterial.shader = LoadShaderFromMemory(LitVertexShader, LitFragmentShader);
    lightDirectionLoc = GetShaderLocation(litMaterial.shader, "lightDirection");
    lightDirection = Vector3Normalize(lightDirection);
}

void RenderSystem::Unload3DResources() {
    for (Mesh& mesh : primitiveMeshes) {
        if (mesh.vaoId != 0) {
            UnloadMesh(mesh);
        }
        mesh = {};
    }

    if (litMaterial.maps != nullptr) {
        UnloadMaterial(litMaterial); // also unloads its shader
    }
    litMaterial = {};
    lightDirectionLoc = -1;
}

void RenderSystem::RecreateTargetTexture() {
    if (targetTexture.id != 0) {
        UnloadRenderTexture(targetTexture);
        targetTexture = {};
    }

    if (IsWindowReady()) {
        targetTexture = LoadRenderTexture(targetWidth, targetHeight);
    }
}

}
