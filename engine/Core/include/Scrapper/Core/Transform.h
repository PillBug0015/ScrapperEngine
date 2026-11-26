#pragma once

#include "Scrapper/Core/Math.h"

#include <entt/entity/registry.hpp>

#include <vector>


namespace Scrapper {
// Local pose relative to the Parent (or the world). Y up, forward is -Z.
// 2D and UI use the same type: x/y on screen, rotation around Z.
struct LocalTransform {
    Vector3 position = { 0.0f, 0.0f, 0.0f };
    Quaternion rotation = { 0.0f, 0.0f, 0.0f, 1.0f };
    Vector3 scale = { 1.0f, 1.0f, 1.0f };
};

// Derived from LocalTransform + Parent by Core's propagation passes. Read it; never write it.
struct WorldTransform {
    Matrix matrix = MatrixIdentity();
    Vector3 position = { 0.0f, 0.0f, 0.0f };
    Quaternion rotation = { 0.0f, 0.0f, 0.0f, 1.0f };
    Vector3 scale = { 1.0f, 1.0f, 1.0f };
};

// The only record of the hierarchy: a child points at its parent. Change it through Transforms::SetParent.
struct Parent {
    entt::entity entity = entt::null;
};

namespace Transforms {
// entt::null detaches. A parent that would create a cycle is refused.
void SetParent(entt::registry& registry, entt::entity child, entt::entity parent);
entt::entity GetParent(const entt::registry& registry, entt::entity entity);
std::vector<entt::entity> Children(const entt::registry& registry, entt::entity entity);

// Always current: walks the parent chain. WorldTransform is as of the last propagation pass.
WorldTransform ComputeWorld(const entt::registry& registry, entt::entity entity);
void SetWorldPose(entt::registry& registry, entt::entity entity, Vector3 position, Quaternion rotation);
void LookAt(entt::registry& registry, entt::entity entity, Vector3 target, Vector3 up = { 0.0f, 1.0f, 0.0f });

inline Vector3 Forward(Quaternion rotation) { return Vector3RotateByQuaternion({ 0.0f, 0.0f, -1.0f }, rotation); }
inline Vector3 Up(Quaternion rotation) { return Vector3RotateByQuaternion({ 0.0f, 1.0f, 0.0f }, rotation); }
inline Vector3 Right(Quaternion rotation) { return Vector3RotateByQuaternion({ 1.0f, 0.0f, 0.0f }, rotation); }
}

}
