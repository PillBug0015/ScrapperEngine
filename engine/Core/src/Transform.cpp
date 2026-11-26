#include "Scrapper/Core/Transform.h"
#include "Scrapper/Core/Lifecycle.h"
#include "HierarchyState.h"

#include <algorithm>
#include <unordered_map>


namespace Scrapper {
namespace {
constexpr int MaxDepth = 64;

Matrix LocalMatrix(const LocalTransform& t) {
    return MatrixMultiply(MatrixMultiply(MatrixScale(t.scale.x, t.scale.y, t.scale.z), QuaternionToMatrix(t.rotation)),
                          MatrixTranslate(t.position.x, t.position.y, t.position.z));
}

WorldTransform Compose(const LocalTransform& local, const WorldTransform* parent) {
    WorldTransform world;
    const Matrix localMatrix = LocalMatrix(local);
    if (parent != nullptr) {
        world.matrix = MatrixMultiply(localMatrix, parent->matrix);
        world.rotation = QuaternionNormalize(QuaternionMultiply(parent->rotation, local.rotation));
        world.scale = Vector3Multiply(parent->scale, local.scale);
    } else {
        world.matrix = localMatrix;
        world.rotation = local.rotation;
        world.scale = local.scale;
    }
    world.position = { world.matrix.m12, world.matrix.m13, world.matrix.m14 };
    return world;
}

entt::entity ValidParent(const entt::registry& registry, entt::entity entity) {
    const Parent* parent = registry.try_get<Parent>(entity);
    if (parent == nullptr || !registry.valid(parent->entity) || !registry.all_of<LocalTransform>(parent->entity)) {
        return entt::null;
    }
    return parent->entity;
}

void RebuildOrder(entt::registry& registry, HierarchyState& state) {
    std::unordered_map<entt::entity, int> depth;
    const auto depthOf = [&](entt::entity start) {
        int d = 0;
        for (entt::entity e = ValidParent(registry, start); e != entt::null && d < MaxDepth; e = ValidParent(registry, e)) {
            if (auto known = depth.find(e); known != depth.end()) {
                d += known->second + 1;
                break;
            }
            ++d;
        }
        return d;
    };

    state.order.clear();
    for (entt::entity entity : registry.view<LocalTransform>()) {
        depth[entity] = depthOf(entity);
        state.order.push_back(entity);
    }
    std::stable_sort(state.order.begin(), state.order.end(),
                     [&](entt::entity a, entt::entity b) { return depth[a] < depth[b]; });

    state.orderDirty = false;
    state.activityDirty = true;
}

void ResolveActivity(entt::registry& registry, HierarchyState& state) {
    const auto apply = [&](entt::entity entity, bool disabled) {
        if (disabled && !registry.all_of<Disabled>(entity)) registry.emplace<Disabled>(entity);
        if (!disabled && registry.all_of<Disabled>(entity)) registry.remove<Disabled>(entity);
    };

    // Parents come first in the order, so their Disabled is already settled.
    for (entt::entity entity : state.order) {
        const entt::entity parent = ValidParent(registry, entity);
        apply(entity, registry.all_of<Inactive>(entity) || (parent != entt::null && registry.all_of<Disabled>(parent)));
    }

    // Entities outside the transform hierarchy only have themselves to consider.
    for (entt::entity entity : registry.view<Inactive>(entt::exclude<LocalTransform>)) apply(entity, true);
    std::vector<entt::entity> revived;
    for (entt::entity entity : registry.view<Disabled>(entt::exclude<LocalTransform, Inactive>)) revived.push_back(entity);
    for (entt::entity entity : revived) apply(entity, false);

    state.activityDirty = false;
}
}

void PropagateTransforms(entt::registry& registry) {
    HierarchyState& state = registry.ctx().get<HierarchyState>();
    if (state.orderDirty) RebuildOrder(registry, state);
    if (state.activityDirty) ResolveActivity(registry, state);

    for (entt::entity entity : state.order) {
        const entt::entity parent = ValidParent(registry, entity);
        registry.get<WorldTransform>(entity) =
            Compose(registry.get<LocalTransform>(entity), parent != entt::null ? &registry.get<WorldTransform>(parent) : nullptr);
    }
}

namespace Transforms {
void SetParent(entt::registry& registry, entt::entity child, entt::entity parent) {
    for (entt::entity e = parent; e != entt::null; e = ValidParent(registry, e)) {
        if (e == child) return; // would make a cycle
    }

    if (parent == entt::null) registry.remove<Parent>(child);
    else registry.emplace_or_replace<Parent>(child, parent);
}

entt::entity GetParent(const entt::registry& registry, entt::entity entity) {
    return ValidParent(registry, entity);
}

std::vector<entt::entity> Children(const entt::registry& registry, entt::entity entity) {
    std::vector<entt::entity> children;
    for (auto [child, parent] : registry.view<Parent>().each()) {
        if (parent.entity == entity) children.push_back(child);
    }
    return children;
}

WorldTransform ComputeWorld(const entt::registry& registry, entt::entity entity) {
    const entt::entity parent = ValidParent(registry, entity);
    if (parent == entt::null) {
        return Compose(registry.get<LocalTransform>(entity), nullptr);
    }
    const WorldTransform parentWorld = ComputeWorld(registry, parent);
    return Compose(registry.get<LocalTransform>(entity), &parentWorld);
}

void SetWorldPose(entt::registry& registry, entt::entity entity, Vector3 position, Quaternion rotation) {
    LocalTransform& local = registry.get<LocalTransform>(entity);
    const entt::entity parent = ValidParent(registry, entity);
    if (parent == entt::null) {
        local.position = position;
        local.rotation = QuaternionNormalize(rotation);
        return;
    }

    const WorldTransform parentWorld = ComputeWorld(registry, parent);
    const Quaternion inverseParent = QuaternionInvert(parentWorld.rotation);
    const Vector3 relative = Vector3RotateByQuaternion(Vector3Subtract(position, parentWorld.position), inverseParent);
    local.position = Vector3Divide(relative, parentWorld.scale);
    local.rotation = QuaternionNormalize(QuaternionMultiply(inverseParent, rotation));
}

void LookAt(entt::registry& registry, entt::entity entity, Vector3 target, Vector3 up) {
    const Vector3 position = ComputeWorld(registry, entity).position;
    const Vector3 forward = Vector3Normalize(Vector3Subtract(target, position));

    Vector3 right = Vector3CrossProduct(forward, up);
    if (Vector3LengthSqr(right) < 1e-6f) {
        right = Vector3CrossProduct(forward, { 0.0f, 0.0f, 1.0f }); // looking straight along up
    }
    right = Vector3Normalize(right);
    const Vector3 trueUp = Vector3CrossProduct(right, forward);

    // Rotation matrix columns are the rotated basis axes: right, up, back (forward is -Z).
    Matrix basis = MatrixIdentity();
    basis.m0 = right.x;    basis.m1 = right.y;    basis.m2 = right.z;
    basis.m4 = trueUp.x;   basis.m5 = trueUp.y;   basis.m6 = trueUp.z;
    basis.m8 = -forward.x; basis.m9 = -forward.y; basis.m10 = -forward.z;

    SetWorldPose(registry, entity, position, QuaternionFromMatrix(basis));
}
}

}
