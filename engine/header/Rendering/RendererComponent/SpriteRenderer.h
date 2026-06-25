#pragma once

#include "Rendering/RendererComponent/Renderer.h"


namespace ScrapperEngine {
class SpriteRenderer : public Renderer {
public:
    Texture2D texture = {};
    Rectangle sourceRect = { 0.0f, 0.0f, 0.0f, 0.0f };
    Vector2 size = { 0.0f, 0.0f };
    Color tint = WHITE;
    Vector2 pivot = { 0.5f, 0.5f };

    SpriteRenderer() = default;

    SpriteRenderer(Texture2D texture, int layer = 0)
        : texture(texture)
    {
        this->layer = layer;
        sourceRect = { 0.0f, 0.0f, static_cast<float>(texture.width), static_cast<float>(texture.height) };
        size = { static_cast<float>(texture.width), static_cast<float>(texture.height) };
    }

    void SetTexture(Texture2D newTexture) {
        texture = newTexture;
        sourceRect = { 0.0f, 0.0f, static_cast<float>(texture.width), static_cast<float>(texture.height) };
        size = { static_cast<float>(texture.width), static_cast<float>(texture.height) };
    }

protected:
    void SubmitRenderCommand() override {
        if (texture.id == 0) {
            return;
        }

        RenderCommand cmd;
        cmd.type = RenderType::TEXTURE;
        cmd.position = GetRenderPosition();
        cmd.rotation = GetRenderRotation();
        cmd.scale = GetRenderScale();
        cmd.size = size;
        cmd.texture = texture;
        cmd.sourceRect = sourceRect;
        cmd.color = tint;
        cmd.layer = layer;
        cmd.pivot = pivot;

        RenderSystem::Instance().Submit(cmd);
    }
};

}
