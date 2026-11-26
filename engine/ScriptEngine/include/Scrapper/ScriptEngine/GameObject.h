#pragma once

#include "Scrapper/Core/Lifecycle.h"
#include "Scrapper/Core/Name.h"
#include "Scrapper/Core/Transform.h"
#include "Scrapper/Core/World.h"

#include <string>
#include <utility>


namespace Scrapper {
// Object-oriented handle over an entity, for game code. It holds an id, never a pointer into the
// component arrays, so it is safe to keep: once the entity is gone, IsValid() turns false.
class GameObject {
public:
    GameObject() = default;
    GameObject(World& world, entt::entity entity) : world(&world), entity(entity) {}

    static GameObject Create(World& world, Vector3 position = { 0.0f, 0.0f, 0.0f });
    static GameObject Create(World& world, Vector3 position, GameObject parent);
    static GameObject CreateEmpty(World& world); // no LocalTransform: managers, network glue

    bool IsValid() const { return world != nullptr && world->Registry().valid(entity); }
    explicit operator bool() const { return IsValid(); }
    bool operator==(const GameObject& other) const { return world == other.world && entity == other.entity; }
    bool operator!=(const GameObject& other) const { return !(*this == other); }

    entt::entity Entity() const { return entity; }
    World& GetWorld() const { return *world; }

    // Data on the entity: any component of any module.
    template <typename T, typename... Args>
    T& Add(Args&&... args) { return world->Registry().template emplace_or_replace<T>(entity, std::forward<Args>(args)...); }
    template <typename T>
    T& Get() const { return world->Registry().template get<T>(entity); }
    template <typename T>
    T* TryGet() const { return world->Registry().template try_get<T>(entity); }
    template <typename T>
    bool Has() const { return world->Registry().template all_of<T>(entity); }
    template <typename T>
    void Remove() { world->Registry().template remove<T>(entity); }

    // LocalTransform: world space unless the name says Local. Reads are always current.
    Vector3 GetPosition() const;
    void SetPosition(Vector3 position);
    Quaternion GetRotation() const;
    void SetRotation(Quaternion rotation);
    Vector3 GetLocalPosition() const;
    void SetLocalPosition(Vector3 position);
    void SetLocalEulerDegrees(Vector3 degrees);
    void LookAt(Vector3 target);
    Vector3 Forward() const;
    Vector3 Up() const;
    Vector3 Right() const;

    // Hierarchy and lifetime
    std::string GetName() const;
    void SetName(std::string name);
    GameObject FindChild(const std::string& name) const; // searches all descendants
    void SetParent(GameObject parent);
    GameObject GetParent() const;
    void SetActive(bool active);
    bool IsActiveSelf() const;
    bool IsActiveInHierarchy() const;
    void Destroy(); // at the end of the frame, children included

    // Scripts (defined in Script.h)
    template <typename T, typename... Args>
    T& AddScript(Args&&... args);
    template <typename T>
    T* GetScript() const;

private:
    World* world = nullptr;
    entt::entity entity = entt::null;
};

}
