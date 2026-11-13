#pragma once

#include "Core/TransformSystem/TransformComponent.h"
#include "Rendering/RenderSystem.h"

#include <raylib.h>
#include <cmath>


namespace ScrapperEngine {
// Common 2D transform: local TRS, lazily cached world TRS.
// Subclasses only decide where the node sits relative to its parent (or the screen when root).
class Transform2DBase : public TransformComponent {
public:
    void SetParent(Transform2DBase* newParent) { AttachTo(newParent); }
    Transform2DBase* GetParent2D() const { return static_cast<Transform2DBase*>(GetParent()); }

    Vector2 GetLocalPosition() const { return localPosition; }
    void SetLocalPosition(Vector2 position) {
        localPosition = position;
        MakeDirty();
    }

    float GetLocalRotation() const { return localRotation; }
    void SetLocalRotation(float degrees) {
        localRotation = degrees;
        MakeDirty();
    }

    Vector2 GetLocalScale() const { return localScale; }
    void SetLocalScale(Vector2 scale) {
        localScale = scale;
        MakeDirty();
    }

    void Translate(Vector2 delta) {
        SetLocalPosition({ localPosition.x + delta.x, localPosition.y + delta.y });
    }

    void Rotate(float degrees) {
        SetLocalRotation(localRotation + degrees);
    }

    Vector2 GetWorldPosition() const { Refresh(); return worldPosition; }
    float GetWorldRotation() const { Refresh(); return worldRotation; }
    Vector2 GetWorldScale() const { Refresh(); return worldScale; }

protected:
    virtual Vector2 RootPosition() const { return localPosition; }
    virtual Vector2 OffsetInParent() const { return localPosition; }

private:
    Vector2 localPosition = { 0.0f, 0.0f };
    float localRotation = 0.0f; // degrees
    Vector2 localScale = { 1.0f, 1.0f };

    mutable Vector2 worldPosition = { 0.0f, 0.0f };
    mutable float worldRotation = 0.0f;
    mutable Vector2 worldScale = { 1.0f, 1.0f };

    void Refresh() const {
        if (!IsDirty()) {
            return;
        }

        const Transform2DBase* parent = GetParent2D();
        if (parent == nullptr) {
            worldPosition = RootPosition();
            worldRotation = localRotation;
            worldScale = localScale;
        } else {
            const Vector2 parentPosition = parent->GetWorldPosition();
            const Vector2 parentScale = parent->GetWorldScale();
            const float parentRotation = parent->GetWorldRotation();

            const Vector2 offset = OffsetInParent();
            const Vector2 scaled = { offset.x * parentScale.x, offset.y * parentScale.y };
            const float cosRot = std::cos(parentRotation * DEG2RAD);
            const float sinRot = std::sin(parentRotation * DEG2RAD);

            worldPosition = {
                parentPosition.x + scaled.x * cosRot - scaled.y * sinRot,
                parentPosition.y + scaled.x * sinRot + scaled.y * cosRot
            };
            worldRotation = parentRotation + localRotation;
            worldScale = { parentScale.x * localScale.x, parentScale.y * localScale.y };
        }

        ClearDirty();
    }
};

// 2D world-space node.
class Transform2D : public Transform2DBase {
};

// 2D screen/UI node. Roots are anchored to the render target, children to their parent rect.
class RectTransform : public Transform2DBase {
public:
    Vector2 size = { 100.0f, 100.0f };
    Vector2 pivot = { 0.5f, 0.5f };
    Vector2 anchor = { 0.0f, 0.0f };

    RectTransform() = default;
    RectTransform(Vector2 size, Vector2 pivot = { 0.5f, 0.5f })
        : size(size), pivot(pivot) {}

    Vector2 GetAnchoredPosition() const { return GetLocalPosition(); }
    void SetAnchoredPosition(Vector2 position) { SetLocalPosition(position); }

    void SetAnchor(Vector2 newAnchor) {
        anchor = newAnchor;
        MakeDirty();
    }

protected:
    Vector2 RootPosition() const override {
        const Vector2 target = RenderSystem::Instance().GetTargetResolution();
        const Vector2 local = GetLocalPosition();
        return { target.x * anchor.x + local.x, target.y * anchor.y + local.y };
    }

    Vector2 OffsetInParent() const override {
        Vector2 offset = GetLocalPosition();
        if (const auto* parentRect = dynamic_cast<const RectTransform*>(GetParent2D())) {
            offset.x += parentRect->size.x * (anchor.x - parentRect->pivot.x);
            offset.y += parentRect->size.y * (anchor.y - parentRect->pivot.y);
        }
        return offset;
    }
};

}
