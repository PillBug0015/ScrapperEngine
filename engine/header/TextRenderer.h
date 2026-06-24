#pragma once

#include "Renderer.h"


namespace ScrapperEngine {
class TextRenderer : public Renderer {
public:
    std::string text = "";
    int fontSize = 20;
    Color color = WHITE;

    TextRenderer(const std::string& text = "", int fontSize = 20, Color color = WHITE, int layer = 0)
        : text(text), fontSize(fontSize), color(color)
    {
        this->layer = layer;
    }

protected:
    void SubmitRenderCommand() override {
        RenderCommand cmd;
        cmd.type = RenderType::TEXT;
        cmd.position = GetRenderPosition();
        cmd.rotation = GetRenderRotation();
        cmd.scale = GetRenderScale();
        cmd.text = text;
        cmd.fontSize = static_cast<int>(fontSize * ((cmd.scale.x + cmd.scale.y) / 2.0f));
        cmd.color = color;
        cmd.layer = layer;

        RenderSystem::Instance().Submit(cmd);
    }
};

}
