#pragma once

#include "Rendering/RendererComponent/Renderer.h"
#include "Core/TransformSystem/Transform2D.h"

#include <cmath>


namespace ScrapperEngine {
// Renderer living on a Transform2D (world) or RectTransform (UI).
class Renderer2D : public Renderer {
public:
    Vector2 offset = { 0.0f, 0.0f };
    Vector2 scale = { 1.0f, 1.0f };

protected:
    RenderBucket ResolveBucket() const override {
        const Transform2DBase* transform = GetTransform();
        if (transform == nullptr) {
            return RenderBucket::None;
        }
        return dynamic_cast<const RectTransform*>(transform) ? RenderBucket::UI : RenderBucket::World2D;
    }

    TransformComponent* GetTransformComponent() const override {
        return GetTransform();
    }

    Transform2DBase* GetTransform() const {
        if (cachedTransform == nullptr && owner != nullptr) {
            cachedTransform = owner->GetComponent<Transform2DBase>();
        }
        return cachedTransform;
    }

    Vector2 GetRenderPosition() const {
        const Transform2DBase* transform = GetTransform();
        const Vector2 worldPos = transform->GetWorldPosition();
        const Vector2 worldScale = transform->GetWorldScale();
        const float worldRotRad = transform->GetWorldRotation() * DEG2RAD;

        const float cosRot = std::cos(worldRotRad);
        const float sinRot = std::sin(worldRotRad);
        const Vector2 scaledOffset = { offset.x * worldScale.x, offset.y * worldScale.y };

        return {
            worldPos.x + scaledOffset.x * cosRot - scaledOffset.y * sinRot,
            worldPos.y + scaledOffset.x * sinRot + scaledOffset.y * cosRot
        };
    }

    float GetRenderRotation() const {
        return GetTransform()->GetWorldRotation();
    }

    Vector2 GetRenderScale() const {
        const Vector2 worldScale = GetTransform()->GetWorldScale();
        return { scale.x * worldScale.x, scale.y * worldScale.y };
    }

private:
    mutable Transform2DBase* cachedTransform = nullptr;
};

}
