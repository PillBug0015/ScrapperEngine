#pragma once

#include "Scrapper/Jolt/JoltWorld.h"

#include <Jolt/Physics/Constraints/Constraint.h>

#include <entt/entity/registry.hpp>


namespace Scrapper {
// Runtime of a Grip, owned by the Jolt module: the joint and the target it was made for.
struct GripJoint {
    JPH::Ref<JPH::Constraint> constraint;
    entt::entity target = entt::null;
};

namespace GripJoints {
// Before each frame's steps: joints for new grips, none for grips that ended or lost a body.
void Sync(entt::registry& registry, JoltWorld& jolt);
void OnDestroyed(entt::registry& registry, entt::entity entity);
}

}
