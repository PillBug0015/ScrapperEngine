#pragma once

#include "CameraComponent.h"
#include "GameObject.h"
#include "Transform.h"

class Camera2DComponent : public CameraComponent {
public:
    Vector2 offset = { 0.0f, 0.0f };
    float zoom = 1.0f;

    bool Begin() override {
        if (owner == nullptr) {
            return false;
        }

        Transform2D* transform = owner->GetComponent<Transform2D>();
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
};
