#pragma once

#include "Transform.h"
#include "Component.h"
#include <vector>
#include <memory>
#include <type_traits>
#include <utility>

class GameObject {
public:
    GameObject() = default;

    TransformComponent* GetTransform() const {
        return GetComponent<TransformComponent>();
    }
    ~GameObject() {
        DestroyComponents();
    }

    // Prevent copying to maintain unique ownership of components
    GameObject(const GameObject&) = delete;
    GameObject& operator=(const GameObject&) = delete;

    // Prevent moving to maintain hierarchy and unique pointer integrity
    GameObject(GameObject&&) = delete;
    GameObject& operator=(GameObject&&) = delete;

    // Add a component of type T
    template <typename T, typename... Args>
    T* AddComponent(Args&&... args) {
        static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");

        auto comp = std::make_unique<T>(std::forward<Args>(args)...);
        comp->owner = this;
        T* ptr = comp.get();

        components.push_back(std::move(comp));
        
        if (activeInHierarchy && !destroyed) {
            ptr->StartLifecycle();
        }

        return ptr;
    }

    // Retrieve a component of type T
    template <typename T>
    T* GetComponent() const {
        static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");

        for (const auto& comp : components) {
            T* ptr = dynamic_cast<T*>(comp.get());
            if (ptr) return ptr;
        }
        return nullptr;
    }

    // Retrieve all components of type T in this GameObject and all its descendants
    template <typename T>
    std::vector<T*> GetComponentsInChildren() const {
        static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");
        std::vector<T*> results;
        GetComponentsInChildrenInternal<T>(results);
        return results;
    }

    void Update(float deltaTime) {
        if (!CanRunComponents()) {
            return;
        }

        for (auto& comp : components) {
            if (comp && comp->IsEnabled()) {
                comp->Update(deltaTime);
            }
        }
    }



    bool IsActive() const {
        return activeSelf;
    }

    bool IsActiveInHierarchy() const {
        return activeInHierarchy && !destroyed;
    }

    bool IsDestroyed() const {
        return destroyed;
    }

    void SetActive(bool newActive) {
        if (destroyed || activeSelf == newActive) {
            return;
        }

        activeSelf = newActive;
        SetActiveInHierarchy(newActive);
    }

    void Destroy() {
        if (destroyed) {
            return;
        }

        const bool wasActiveInHierarchy = activeInHierarchy;
        destroyed = true;

        TransformComponent* trans = GetTransform();
        if (trans) {
            for (auto* childTrans : trans->children) {
                if (childTrans && childTrans->owner) {
                    childTrans->owner->Destroy();
                }
            }
        }

        DestroyComponents(wasActiveInHierarchy);
    }

private:
    template <typename T>
    void GetComponentsInChildrenInternal(std::vector<T*>& outResults) const {
        // 1. Check components on this GameObject
        for (const auto& comp : components) {
            T* ptr = dynamic_cast<T*>(comp.get());
            if (ptr) {
                outResults.push_back(ptr);
            }
        }
        // 2. Check child transforms recursively
        const TransformComponent* trans = GetTransform();
        if (trans) {
            for (const auto* childTrans : trans->children) {
                if (childTrans && childTrans->owner) {
                    childTrans->owner->GetComponentsInChildrenInternal<T>(outResults);
                }
            }
        }
    }

    std::vector<std::unique_ptr<Component>> components;
    bool activeSelf = true;
    bool activeInHierarchy = true;
    bool destroyed = false;

    bool CanRunComponents() const {
        return activeInHierarchy && !destroyed;
    }

    void SetActiveInHierarchy(bool newActiveInHierarchy) {
        if (activeInHierarchy == newActiveInHierarchy) {
            return;
        }

        activeInHierarchy = newActiveInHierarchy;

        for (auto& comp : components) {
            if (!comp || comp->IsDestroyed() || !comp->IsRawEnabled() || !comp->HasStarted()) {
                continue;
            }

            if (activeInHierarchy) {
                comp->OnEnable();
            } else {
                comp->OnDisable();
            }
        }
    }

    void DestroyComponents(bool ownerActiveInHierarchy = false) {
        for (auto& comp : components) {
            if (comp && !comp->IsDestroyed()) {
                comp->DestroyLifecycle(ownerActiveInHierarchy);
            }
        }

        components.clear();
    }
};
