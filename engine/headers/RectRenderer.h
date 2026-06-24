#pragma once

#include "Renderer.h"


namespace ScrapperEngine {
class RectRenderer : public Renderer {
public:
    Vector2 size = { 0.0f, 0.0f };
    Color color = WHITE;
    Vector2 pivot = { 0.5f, 0.5f };

    RectRenderer(Vector2 size, Color color, int layer = 0)
        : size(size), color(color)
    {
        this->layer = layer;
    }

protected:
    void SubmitRenderCommand() override {
        RenderCommand cmd;
        cmd.type = RenderType::RECTANGLE;
        cmd.position = GetRenderPosition();
        cmd.rotation = GetRenderRotation();
        cmd.scale = GetRenderScale();
        cmd.size = size;
        cmd.color = color;
        cmd.layer = layer;
        cmd.pivot = pivot;

        RenderSystem::Instance().Submit(cmd);
    }
};

}
