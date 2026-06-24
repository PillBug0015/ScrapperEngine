#pragma once

#include "Component.h"
#include "GameObject.h"
#include "RenderSystem.h"

#include <cmath>

class Renderer : public Component {
public:
    bool visible = true;
    int layer = 0;

    Vector2 offset = { 0.0f, 0.0f };
    Vector2 scale = { 1.0f, 1.0f };

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

        TransformComponent* trans = owner->GetComponent<TransformComponent>();
        if (trans == nullptr || !trans->IsEnabled()) {
            return; // Skip rendering completely!
        }

        SubmitRenderCommand();
    }

protected:
    virtual void SubmitRenderCommand() = 0;

    Vector2 GetRenderPosition() const {
        TransformComponent* trans = owner->GetComponent<TransformComponent>();
        if (trans == nullptr) {
            return offset;
        }

        Vector2 worldPos = trans->GetWorldPosition();
        Vector2 worldScale = trans->GetWorldScale();
        float worldRotRad = trans->GetWorldRotation() * DEG2RAD;

        float cosRot = std::cos(worldRotRad);
        float sinRot = std::sin(worldRotRad);

        Vector2 scaledOffset = { offset.x * worldScale.x, offset.y * worldScale.y };
        Vector2 rotatedOffset = {
            scaledOffset.x * cosRot - scaledOffset.y * sinRot,
            scaledOffset.x * sinRot + scaledOffset.y * cosRot
        };

        return { worldPos.x + rotatedOffset.x, worldPos.y + rotatedOffset.y };
    }

    float GetRenderRotation() const {
        TransformComponent* trans = owner->GetComponent<TransformComponent>();
        if (trans == nullptr) {
            return 0.0f;
        }
        return trans->GetWorldRotation();
    }

    Vector2 GetRenderScale() const {
        TransformComponent* trans = owner->GetComponent<TransformComponent>();
        if (trans == nullptr) {
            return scale;
        }
        Vector2 worldScale = trans->GetWorldScale();
        return { scale.x * worldScale.x, scale.y * worldScale.y };
    }

private:
    int renderIndex = -1;
    RenderBucket renderBucket = RenderBucket::None;

    friend class RenderSystem;
};
