#pragma once

#include "Rendering/CameraComponent/CameraComponent.h"
#include "Core/GameObject.h"
#include "Core/TransformSystem/Transform.h"


namespace ScrapperEngine {
class Camera2DComponent : public CameraComponent {
public:
    Vector2 offset = { 0.0f, 0.0f };
    float zoom = 1.0f;

    bool Begin() override {
        if (owner == nullptr) {
            return false;
        }

        // Lazy-caching pointer lookup: O(1) performance after first frame
        TransformComponent* transform = GetTransform();
        if (transform == nullptr || !transform->IsEnabled()) {
            return false;
        }

        camera.offset = offset;
        camera.target = transform->GetWorldPosition();
        camera.rotation = transform->GetWorldRotation();
        camera.zoom = zoom;

        BeginMode2D(camera);
        return true;
    }

    void End() override {
        EndMode2D();
    }

private:
    ::Camera2D camera = { { 0.0f, 0.0f }, { 0.0f, 0.0f }, 0.0f, 1.0f };
    mutable TransformComponent* cachedTransform = nullptr;

    TransformComponent* GetTransform() const {
        // Keeps attempting to cache until the sibling TransformComponent is found
        if (cachedTransform == nullptr && owner != nullptr) {
            cachedTransform = owner->GetComponent<TransformComponent>();
        }
        return cachedTransform;
    }
};

}
