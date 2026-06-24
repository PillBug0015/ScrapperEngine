#pragma once

#include "Component.h"
#include <raylib.h>
#include <vector>
#include <algorithm>
#include <cmath>


namespace ScrapperEngine {
class GameObject;

class TransformComponent : public Component {
public:
    TransformComponent* parent = nullptr;
    std::vector<TransformComponent*> children;

    virtual ~TransformComponent() {
        // Safe detachment to prevent dangling pointers
        if (parent) {
            parent->RemoveChild(this);
        }
        for (TransformComponent* child : children) {
            if (child) {
                child->parent = nullptr;
            }
        }
    }

    // Prevent copying to maintain hierarchy integrity
    TransformComponent(const TransformComponent&) = delete;
    TransformComponent& operator=(const TransformComponent&) = delete;
    TransformComponent(TransformComponent&&) = delete;
    TransformComponent& operator=(TransformComponent&&) = delete;

    TransformComponent() = default;

    // --- Hierarchy Management ---
    void SetParent(TransformComponent* newParent) {
        if (parent == newParent) return;

        // Prevent circular hierarchy (e.g., setting a descendant as parent)
        TransformComponent* temp = newParent;
        while (temp != nullptr) {
            if (temp == this) return; // Ignore circular reference
            temp = temp->parent;
        }

        // Remove from old parent's children list
        if (parent) {
            parent->RemoveChild(this);
        }

        parent = newParent;

        // Add to new parent's children list
        if (parent) {
            parent->children.push_back(this);
        }
        
        // Relationship changed, must recalculate world transform
        MakeDirty();
    }

    void RemoveChild(TransformComponent* child) {
        if (!child) return;
        auto it = std::find(children.begin(), children.end(), child);
        if (it != children.end()) {
            children.erase(it);
            child->parent = nullptr;
            child->MakeDirty();
        }
    }

    // --- Common 2D Interface ---
    virtual Vector2 GetLocalPosition() const = 0;
    virtual void SetLocalPosition(Vector2 pos) = 0;
    virtual float GetLocalRotation() const = 0;
    virtual void SetLocalRotation(float rot) = 0;
    virtual Vector2 GetLocalScale() const = 0;
    virtual void SetLocalScale(Vector2 scl) = 0;

    virtual Vector2 GetWorldPosition() const = 0;
    virtual float GetWorldRotation() const = 0;
    virtual Vector2 GetWorldScale() const = 0;

    virtual void MakeDirty() const = 0;
};

#pragma region Transform2D
// ==========================================
// Transform2D - 2D World Space Transform
// ==========================================
class Transform2D : public TransformComponent {
public:
    Transform2D() = default;

    Vector2 GetLocalPosition() const override { return localPosition; }
    void SetLocalPosition(Vector2 pos) override {
        localPosition = pos;
        MakeDirty();
    }

    float GetLocalRotation() const override { return localRotation; }
    void SetLocalRotation(float rot) override {
        localRotation = rot;
        MakeDirty();
    }

    Vector2 GetLocalScale() const override { return localScale; }
    void SetLocalScale(Vector2 scl) override {
        localScale = scl;
        MakeDirty();
    }

    void Translate(Vector2 delta) {
        SetLocalPosition({ localPosition.x + delta.x, localPosition.y + delta.y });
    }

    void Rotate(float degrees) {
        SetLocalRotation(localRotation + degrees);
    }

    Vector2 GetWorldPosition() const override {
        if (isDirty) {
            RecalculateWorldTransform();
        }
        return cachedWorldPosition;
    }

    float GetWorldRotation() const override {
        if (isDirty) {
            RecalculateWorldTransform();
        }
        return cachedWorldRotation;
    }

    Vector2 GetWorldScale() const override {
        if (isDirty) {
            RecalculateWorldTransform();
        }
        return cachedWorldScale;
    }

    void MakeDirty() const override {
        if (isDirty) return; // Already dirty

        isDirty = true;
        for (TransformComponent* child : children) {
            if (child) {
                child->MakeDirty();
            }
        }
    }

private:
    Vector2 localPosition = { 0.0f, 0.0f };
    float localRotation = 0.0f;             // in degrees
    Vector2 localScale = { 1.0f, 1.0f };

    mutable Vector2 cachedWorldPosition = { 0.0f, 0.0f };
    mutable float cachedWorldRotation = 0.0f;
    mutable Vector2 cachedWorldScale = { 1.0f, 1.0f };
    mutable bool isDirty = true;

    void RecalculateWorldTransform() const {
        if (!parent) {
            cachedWorldPosition = localPosition;
            cachedWorldRotation = localRotation;
            cachedWorldScale = localScale;
        } else {
            Vector2 parentPos = parent->GetWorldPosition();
            Vector2 parentScale = parent->GetWorldScale();
            float parentRotRad = parent->GetWorldRotation() * DEG2RAD;

            Vector2 scaledLocal = { localPosition.x * parentScale.x, localPosition.y * parentScale.y };

            float cosRot = std::cos(parentRotRad);
            float sinRot = std::sin(parentRotRad);
            Vector2 rotatedLocal = {
                scaledLocal.x * cosRot - scaledLocal.y * sinRot,
                scaledLocal.x * sinRot + scaledLocal.y * cosRot
            };

            cachedWorldPosition = { parentPos.x + rotatedLocal.x, parentPos.y + rotatedLocal.y };
            cachedWorldRotation = parent->GetWorldRotation() + localRotation;
            cachedWorldScale = { parentScale.x * localScale.x, parentScale.y * localScale.y };
        }
        isDirty = false;
    }
};
#pragma endregion




#pragma region Tranform3D
// ==========================================
// Transform3D - 3D World Space Transform (Stub)
// ==========================================
class Transform3D : public TransformComponent {
public:
    Transform3D() = default;

    // 3D Specific properties (not yet fully implemented)
    Vector3 position = { 0.0f, 0.0f, 0.0f };
    Vector4 rotation = { 0.0f, 0.0f, 0.0f, 1.0f }; // Quaternion stub
    Vector3 scale = { 1.0f, 1.0f, 1.0f };

    // Stub implementations for the 2D interface
    Vector2 GetLocalPosition() const override { return { position.x, position.y }; }
    void SetLocalPosition(Vector2 pos) override { position.x = pos.x; position.y = pos.y; }
    float GetLocalRotation() const override { return 0.0f; }
    void SetLocalRotation(float /*rot*/) override {}
    Vector2 GetLocalScale() const override { return { scale.x, scale.y }; }
    void SetLocalScale(Vector2 scl) override { scale.x = scl.x; scale.y = scl.y; }

    Vector2 GetWorldPosition() const override { return { position.x, position.y }; }
    float GetWorldRotation() const override { return 0.0f; }
    Vector2 GetWorldScale() const override { return { scale.x, scale.y }; }

    void MakeDirty() const override {}
};
#pragma endregion

#pragma region RectTransform
// ==========================================
// RectTransform - 2D Screen / UI Space Transform
// ==========================================
class RectTransform : public TransformComponent {
public:
    Vector2 size = { 100.0f, 100.0f };
    Vector2 pivot = { 0.5f, 0.5f };

    RectTransform() = default;
    RectTransform(Vector2 size, Vector2 pivot = { 0.5f, 0.5f })
        : size(size), pivot(pivot) {}

    Vector2 GetLocalPosition() const override { return localPosition; }
    void SetLocalPosition(Vector2 pos) override {
        localPosition = pos;
        MakeDirty();
    }

    float GetLocalRotation() const override { return localRotation; }
    void SetLocalRotation(float rot) override {
        localRotation = rot;
        MakeDirty();
    }

    Vector2 GetLocalScale() const override { return localScale; }
    void SetLocalScale(Vector2 scl) override {
        localScale = scl;
        MakeDirty();
    }

    Vector2 GetWorldPosition() const override {
        if (isDirty) {
            RecalculateWorldTransform();
        }
        return cachedWorldPosition;
    }

    float GetWorldRotation() const override {
        if (isDirty) {
            RecalculateWorldTransform();
        }
        return cachedWorldRotation;
    }

    Vector2 GetWorldScale() const override {
        if (isDirty) {
            RecalculateWorldTransform();
        }
        return cachedWorldScale;
    }

    void MakeDirty() const override {
        if (isDirty) return;

        isDirty = true;
        for (TransformComponent* child : children) {
            if (child) {
                child->MakeDirty();
            }
        }
    }

private:
    Vector2 localPosition = { 0.0f, 0.0f };
    float localRotation = 0.0f;
    Vector2 localScale = { 1.0f, 1.0f };

    mutable Vector2 cachedWorldPosition = { 0.0f, 0.0f };
    mutable float cachedWorldRotation = 0.0f;
    mutable Vector2 cachedWorldScale = { 1.0f, 1.0f };
    mutable bool isDirty = true;

    void RecalculateWorldTransform() const {
        if (!parent) {
            cachedWorldPosition = localPosition;
            cachedWorldRotation = localRotation;
            cachedWorldScale = localScale;
        } else {
            Vector2 parentPos = parent->GetWorldPosition();
            Vector2 parentScale = parent->GetWorldScale();
            float parentRotRad = parent->GetWorldRotation() * DEG2RAD;

            Vector2 scaledLocal = { localPosition.x * parentScale.x, localPosition.y * parentScale.y };

            float cosRot = std::cos(parentRotRad);
            float sinRot = std::sin(parentRotRad);
            Vector2 rotatedLocal = {
                scaledLocal.x * cosRot - scaledLocal.y * sinRot,
                scaledLocal.x * sinRot + scaledLocal.y * cosRot
            };

            cachedWorldPosition = { parentPos.x + rotatedLocal.x, parentPos.y + rotatedLocal.y };
            cachedWorldRotation = parent->GetWorldRotation() + localRotation;
            cachedWorldScale = { parentScale.x * localScale.x, parentScale.y * localScale.y };
        }
        isDirty = false;
    }
};
#pragma endregion
}
