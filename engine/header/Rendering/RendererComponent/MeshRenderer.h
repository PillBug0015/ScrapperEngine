#pragma once

#include "Rendering/RendererComponent/Renderer.h"
#include "Core/TransformSystem/Transform3D.h"

#include <raymath.h>


namespace ScrapperEngine {
// Draws a unit primitive on a Transform3D, stretched to `size`.
class MeshRenderer : public Renderer {
public:
    PrimitiveMesh mesh = PrimitiveMesh::Cube;
    Vector3 size = { 1.0f, 1.0f, 1.0f };
    Color color = WHITE;

    MeshRenderer(PrimitiveMesh mesh, Vector3 size = { 1.0f, 1.0f, 1.0f }, Color color = WHITE)
        : mesh(mesh), size(size), color(color) {}

protected:
    RenderBucket ResolveBucket() const override {
        return GetTransform() ? RenderBucket::World3D : RenderBucket::None;
    }

    TransformComponent* GetTransformComponent() const override {
        return GetTransform();
    }

    void SubmitRenderCommand() override {
        RenderCommand3D cmd;
        cmd.mesh = mesh;
        cmd.color = color;
        cmd.transform = MatrixMultiply(MatrixScale(size.x, size.y, size.z), GetTransform()->GetWorldMatrix());

        RenderSystem::Instance().Submit(cmd);
    }

    Transform3D* GetTransform() const {
        if (cachedTransform == nullptr && owner != nullptr) {
            cachedTransform = owner->GetComponent<Transform3D>();
        }
        return cachedTransform;
    }

private:
    mutable Transform3D* cachedTransform = nullptr;
};

}
