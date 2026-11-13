#pragma once

#include "Core/TransformSystem/TransformComponent.h"

#include <raylib.h>
#include <raymath.h>
#include <cmath>


namespace ScrapperEngine {
// 3D node: local TRS (rotation as quaternion), lazily cached world TRS and matrix.
// Convention: Y up, forward is -Z (OpenGL / raylib).
class Transform3D : public TransformComponent {
public:
    void SetParent(Transform3D* newParent) { AttachTo(newParent); }
    Transform3D* GetParent3D() const { return static_cast<Transform3D*>(GetParent()); }

    Vector3 GetLocalPosition() const { return localPosition; }
    void SetLocalPosition(Vector3 position) {
        localPosition = position;
        MakeDirty();
    }

    Quaternion GetLocalRotation() const { return localRotation; }
    void SetLocalRotation(Quaternion rotation) {
        localRotation = QuaternionNormalize(rotation);
        MakeDirty();
    }

    void SetLocalEulerDegrees(Vector3 degrees) {
        SetLocalRotation(QuaternionFromEuler(degrees.x * DEG2RAD, degrees.y * DEG2RAD, degrees.z * DEG2RAD));
    }

    Vector3 GetLocalScale() const { return localScale; }
    void SetLocalScale(Vector3 scale) {
        localScale = scale;
        MakeDirty();
    }

    void Translate(Vector3 delta) {
        SetLocalPosition(Vector3Add(localPosition, delta));
    }

    Vector3 GetWorldPosition() const { Refresh(); return worldPosition; }
    Quaternion GetWorldRotation() const { Refresh(); return worldRotation; }
    Vector3 GetWorldScale() const { Refresh(); return worldScale; }
    Matrix GetWorldMatrix() const { Refresh(); return worldMatrix; }

    Vector3 Forward() const { return Vector3RotateByQuaternion({ 0.0f, 0.0f, -1.0f }, GetWorldRotation()); }
    Vector3 Up() const { return Vector3RotateByQuaternion({ 0.0f, 1.0f, 0.0f }, GetWorldRotation()); }
    Vector3 Right() const { return Vector3RotateByQuaternion({ 1.0f, 0.0f, 0.0f }, GetWorldRotation()); }

    // Sets the world pose, converting it into the parent's space.
    void SetWorldPose(Vector3 position, Quaternion rotation) {
        if (const Transform3D* parent = GetParent3D()) {
            const Quaternion inverseParent = QuaternionInvert(parent->GetWorldRotation());
            const Vector3 relative = Vector3RotateByQuaternion(
                Vector3Subtract(position, parent->GetWorldPosition()), inverseParent);
            localPosition = Vector3Divide(relative, parent->GetWorldScale());
            localRotation = QuaternionNormalize(QuaternionMultiply(inverseParent, rotation));
        } else {
            localPosition = position;
            localRotation = QuaternionNormalize(rotation);
        }

        MakeDirty();
    }

    void LookAt(Vector3 target, Vector3 up = { 0.0f, 1.0f, 0.0f }) {
        const Vector3 position = GetWorldPosition();
        const Vector3 forward = Vector3Normalize(Vector3Subtract(target, position));

        Vector3 right = Vector3CrossProduct(forward, up);
        if (Vector3LengthSqr(right) < 1e-6f) {
            right = Vector3CrossProduct(forward, { 0.0f, 0.0f, 1.0f }); // looking straight along up
        }
        right = Vector3Normalize(right);
        const Vector3 trueUp = Vector3CrossProduct(right, forward);

        // Rotation matrix columns are the rotated basis axes: right, up, back (forward is -Z).
        Matrix basis = MatrixIdentity();
        basis.m0 = right.x;     basis.m1 = right.y;     basis.m2 = right.z;
        basis.m4 = trueUp.x;    basis.m5 = trueUp.y;    basis.m6 = trueUp.z;
        basis.m8 = -forward.x;  basis.m9 = -forward.y;  basis.m10 = -forward.z;

        SetWorldPose(position, QuaternionFromMatrix(basis));
    }

private:
    Vector3 localPosition = { 0.0f, 0.0f, 0.0f };
    Quaternion localRotation = { 0.0f, 0.0f, 0.0f, 1.0f };
    Vector3 localScale = { 1.0f, 1.0f, 1.0f };

    mutable Vector3 worldPosition = { 0.0f, 0.0f, 0.0f };
    mutable Quaternion worldRotation = { 0.0f, 0.0f, 0.0f, 1.0f };
    mutable Vector3 worldScale = { 1.0f, 1.0f, 1.0f };
    mutable Matrix worldMatrix = MatrixIdentity();

    void Refresh() const {
        if (!IsDirty()) {
            return;
        }

        const Matrix local = MatrixMultiply(
            MatrixMultiply(MatrixScale(localScale.x, localScale.y, localScale.z), QuaternionToMatrix(localRotation)),
            MatrixTranslate(localPosition.x, localPosition.y, localPosition.z));

        if (const Transform3D* parent = GetParent3D()) {
            worldMatrix = MatrixMultiply(local, parent->GetWorldMatrix());
            worldRotation = QuaternionNormalize(QuaternionMultiply(parent->GetWorldRotation(), localRotation));
            worldScale = Vector3Multiply(parent->GetWorldScale(), localScale);
        } else {
            worldMatrix = local;
            worldRotation = localRotation;
            worldScale = localScale;
        }

        worldPosition = { worldMatrix.m12, worldMatrix.m13, worldMatrix.m14 };
        ClearDirty();
    }
};

}
