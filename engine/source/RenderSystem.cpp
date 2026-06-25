#include "Rendering/RenderSystem.h"
#include "Rendering/CameraComponent/CameraComponent.h"
#include "Rendering/RendererComponent/Renderer.h"
#include <algorithm>


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
}

void RenderSystem::Shutdown() {
    Clear();

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
    ClearBackground(BLACK);
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

    if (r->owner->GetComponent<RectTransform>() != nullptr) {
        RegisterUIRenderer(r);
        return;
    }

    if (r->owner->GetComponent<Transform2D>() != nullptr) {
        RegisterWorldRenderer(r);
    }
}

void RenderSystem::UnregisterRenderer(Renderer* r) {
    if (r == nullptr || r->renderBucket == RenderBucket::None) {
        return;
    }

    if (r->renderBucket == RenderBucket::World) {
        UnregisterFromBucket(r, worldRenderers);
    } else {
        UnregisterFromBucket(r, uiRenderers);
    }
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
    // 1. Clear temporary command queues
    worldCommands.clear();
    uiCommands.clear();

    // 2. Query active renderers from each dense renderer bucket.
    CollectCommands(worldRenderers, worldCommands);
    CollectCommands(uiRenderers, uiCommands);

    // 3. Sort world commands by layer ascending
    std::sort(worldCommands.begin(), worldCommands.end(), [](const RenderCommand& a, const RenderCommand& b) {
        return a.layer < b.layer;
    });

    // 4. Sort UI commands by layer ascending
    std::sort(uiCommands.begin(), uiCommands.end(), [](const RenderCommand& a, const RenderCommand& b) {
        return a.layer < b.layer;
    });

    // 5. Render world commands through registered cameras in order.
    std::sort(registeredCameras.begin(), registeredCameras.end(), [](const CameraComponent* a, const CameraComponent* b) {
        return a->order < b->order;
    });

    bool renderedWorld = false;
    for (CameraComponent* camera : registeredCameras) {
        if (camera && camera->IsEnabled() && camera->Begin()) {
            ExecuteCommands(worldCommands);
            camera->End();
            renderedWorld = true;
        }
    }

    if (!renderedWorld) {
        ExecuteCommands(worldCommands);
    }

    // 6. Render UI commands (always screen space)
    ExecuteCommands(uiCommands);

    // 7. Clear queues for the next frame
    Clear();
}

void RenderSystem::Submit(const RenderCommand& cmd) {
    if (activeCommandQueue != nullptr) {
        activeCommandQueue->push_back(cmd);
    }
}

void RenderSystem::RegisterWorldRenderer(Renderer* r) {
    r->renderBucket = RenderBucket::World;
    r->renderIndex = static_cast<int>(worldRenderers.size());
    worldRenderers.push_back(r);
}

void RenderSystem::RegisterUIRenderer(Renderer* r) {
    r->renderBucket = RenderBucket::UI;
    r->renderIndex = static_cast<int>(uiRenderers.size());
    uiRenderers.push_back(r);
}

void RenderSystem::UnregisterFromBucket(Renderer* r, std::vector<Renderer*>& renderers) {
    const int index = r->renderIndex;
    Renderer* moved = renderers.back();

    renderers[static_cast<std::size_t>(index)] = moved;
    moved->renderIndex = index;

    renderers.pop_back();

    r->renderIndex = -1;
    r->renderBucket = RenderBucket::None;
}

void RenderSystem::CollectCommands(const std::vector<Renderer*>& renderers, std::vector<RenderCommand>& commands) {
    activeCommandQueue = &commands;

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
