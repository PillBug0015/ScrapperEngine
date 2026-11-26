#include "Scrapper/ScriptEngine/GameObject.h"

#include <vector>


namespace Scrapper {
GameObject GameObject::Create(World& world, Vector3 position) {
    return Create(world, position, GameObject());
}

GameObject GameObject::Create(World& world, Vector3 position, GameObject parent) {
    entt::registry& registry = world.Registry();
    const entt::entity entity = registry.create();
    registry.emplace<LocalTransform>(entity, LocalTransform{ position });
    if (parent.IsValid()) Transforms::SetParent(registry, entity, parent.Entity());
    return { world, entity };
}

GameObject GameObject::CreateEmpty(World& world) {
    return { world, world.Registry().create() };
}

Vector3 GameObject::GetPosition() const { return Transforms::ComputeWorld(world->Registry(), entity).position; }
Quaternion GameObject::GetRotation() const { return Transforms::ComputeWorld(world->Registry(), entity).rotation; }
Vector3 GameObject::GetLocalPosition() const { return Get<LocalTransform>().position; }
void GameObject::SetLocalPosition(Vector3 position) { Get<LocalTransform>().position = position; }

void GameObject::SetPosition(Vector3 position) {
    Transforms::SetWorldPose(world->Registry(), entity, position, GetRotation());
}

void GameObject::SetRotation(Quaternion rotation) {
    Transforms::SetWorldPose(world->Registry(), entity, GetPosition(), rotation);
}

void GameObject::SetLocalEulerDegrees(Vector3 degrees) {
    Get<LocalTransform>().rotation = QuaternionFromEuler(degrees.x * DEG2RAD, degrees.y * DEG2RAD, degrees.z * DEG2RAD);
}

void GameObject::LookAt(Vector3 target) { Transforms::LookAt(world->Registry(), entity, target); }
Vector3 GameObject::Forward() const { return Transforms::Forward(GetRotation()); }
Vector3 GameObject::Up() const { return Transforms::Up(GetRotation()); }
Vector3 GameObject::Right() const { return Transforms::Right(GetRotation()); }

std::string GameObject::GetName() const {
    const Name* name = TryGet<Name>();
    return name != nullptr ? name->value : std::string();
}

void GameObject::SetName(std::string name) { Add<Name>(Name{ std::move(name) }); }

GameObject GameObject::FindChild(const std::string& name) const {
    std::vector<entt::entity> pending = Transforms::Children(world->Registry(), entity);
    while (!pending.empty()) {
        const entt::entity current = pending.back();
        pending.pop_back();
        const Name* found = world->Registry().try_get<Name>(current);
        if (found != nullptr && found->value == name) return { *world, current };
        for (entt::entity child : Transforms::Children(world->Registry(), current)) pending.push_back(child);
    }
    return {};
}

void GameObject::SetParent(GameObject parent) {
    Transforms::SetParent(world->Registry(), entity, parent.IsValid() ? parent.Entity() : entt::null);
}

GameObject GameObject::GetParent() const {
    const entt::entity parent = Transforms::GetParent(world->Registry(), entity);
    return parent != entt::null ? GameObject(*world, parent) : GameObject();
}

void GameObject::SetActive(bool active) { Lifecycle::SetActive(world->Registry(), entity, active); }
bool GameObject::IsActiveSelf() const { return Lifecycle::IsActiveSelf(world->Registry(), entity); }
bool GameObject::IsActiveInHierarchy() const { return Lifecycle::IsActiveInHierarchy(world->Registry(), entity); }
void GameObject::Destroy() { Lifecycle::Destroy(world->Registry(), entity); }

}
