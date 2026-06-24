#include "RenderSystem.h"
#include "CameraComponent.h"
#include "Renderer.h"
#include <algorithm>


namespace ScrapperEngine {
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
                DrawText(cmd.text.c_str(), (int)cmd.position.x, (int)cmd.position.y, cmd.fontSize, cmd.color);
                break;
            }
            case RenderType::LINE: {
                DrawLineEx(cmd.position, cmd.endPosition, cmd.thickness, cmd.color);
                break;
            }
        }
    }
}

}
