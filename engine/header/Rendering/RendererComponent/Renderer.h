#pragma once

#include "Core/Component/Component.h"
#include "Core/GameObject.h"
#include "Rendering/RenderSystem.h"


namespace ScrapperEngine {
class Renderer : public Component {
public:
    bool visible = true;
    int layer = 0;

    // Automatic registration with RenderSystem during lifecycle
    void OnEnable() override {
        RenderSystem::Instance().RegisterRenderer(this);
    }

    void OnDisable() override {
        RenderSystem::Instance().UnregisterRenderer(this);
    }

    void OnDestroy() override {
        RenderSystem::Instance().UnregisterRenderer(this);
    }

    bool IsVisible() const {
        return visible;
    }

    void SetVisible(bool newVisible) {
        visible = newVisible;
    }

    // Called by RenderSystem to draw the renderer
    void Draw() {
        if (!visible || owner == nullptr) {
            return;
        }

        const TransformComponent* transform = GetTransformComponent();
        if (transform == nullptr || !transform->IsEnabled()) {
            return;
        }

        SubmitRenderCommand();
    }

protected:
    // The pass this renderer belongs to, decided by the transform kind it sits on.
    virtual RenderBucket ResolveBucket() const = 0;
    virtual TransformComponent* GetTransformComponent() const = 0;
    virtual void SubmitRenderCommand() = 0;

private:
    RenderBucket renderBucket = RenderBucket::None;
    int renderIndex = -1;

    friend class RenderSystem;
};

}
