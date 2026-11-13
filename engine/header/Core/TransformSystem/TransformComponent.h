#pragma once

#include "Core/Component/Component.h"

#include <algorithm>
#include <vector>


namespace ScrapperEngine {
// Hierarchy and dirty propagation shared by every transform kind.
// Derived transforms expose a typed SetParent, so 2D and 3D trees can never be mixed.
class TransformComponent : public Component {
public:
    TransformComponent() = default;

    virtual ~TransformComponent() {
        if (parent) {
            parent->RemoveChild(this);
        }
        for (TransformComponent* child : children) {
            child->parent = nullptr;
            child->MakeDirty();
        }
    }

    TransformComponent(const TransformComponent&) = delete;
    TransformComponent& operator=(const TransformComponent&) = delete;
    TransformComponent(TransformComponent&&) = delete;
    TransformComponent& operator=(TransformComponent&&) = delete;

    TransformComponent* GetParent() const { return parent; }
    const std::vector<TransformComponent*>& GetChildren() const { return children; }

    void RemoveChild(TransformComponent* child) {
        auto it = std::find(children.begin(), children.end(), child);
        if (it == children.end()) {
            return;
        }

        children.erase(it);
        child->parent = nullptr;
        child->MakeDirty();
    }

    // A dirty node always has dirty descendants, so stopping at an already dirty node is safe.
    void MakeDirty() const {
        if (dirty) {
            return;
        }

        dirty = true;
        for (TransformComponent* child : children) {
            child->MakeDirty();
        }
    }

protected:
    void AttachTo(TransformComponent* newParent) {
        if (parent == newParent) {
            return;
        }

        for (TransformComponent* node = newParent; node != nullptr; node = node->parent) {
            if (node == this) {
                return; // would create a cycle
            }
        }

        if (parent) {
            parent->RemoveChild(this);
        }

        parent = newParent;
        if (parent) {
            parent->children.push_back(this);
        }

        MakeDirty();
    }

    bool IsDirty() const { return dirty; }
    void ClearDirty() const { dirty = false; }

private:
    TransformComponent* parent = nullptr;
    std::vector<TransformComponent*> children;
    mutable bool dirty = true;
};

}
