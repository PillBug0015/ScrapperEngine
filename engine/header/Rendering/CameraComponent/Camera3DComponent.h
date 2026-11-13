#pragma once

#include "Rendering/CameraComponent/CameraComponent.h"
#include "Core/GameObject.h"
#include "Core/TransformSystem/Transform3D.h"

#include <raymath.h>


namespace ScrapperEngine {
// Perspective camera looking along its Transform3D forward (-Z).
class Camera3DComponent : public CameraComponent {
public:
    float fovy = 60.0f;
    int projection = CAMERA_PERSPECTIVE;

    RenderBucket Space() const override {
        return RenderBucket::World3D;
    }

    bool Begin() override {
        const Transform3D* transform = GetTransform();
        if (transform == nullptr || !transform->IsEnabled()) {
            return false;
        }

        Camera3D camera = {};
        camera.position = transform->GetWorldPosition();
        camera.target = Vector3Add(camera.position, transform->Forward());
        camera.up = transform->Up();
        camera.fovy = fovy;
        camera.projection = projection;

        BeginMode3D(camera);
        return true;
    }

    void End() override {
        EndMode3D();
    }

private:
    mutable Transform3D* cachedTransform = nullptr;

    Transform3D* GetTransform() const {
        if (cachedTransform == nullptr && owner != nullptr) {
            cachedTransform = owner->GetComponent<Transform3D>();
        }
        return cachedTransform;
    }
};

}
