#pragma once

#include "Scrapper/Jolt/JoltWorld.h"
#include "Scrapper/Jolt/PhysicsBody.h"
#include "Scrapper/Jolt/Ragdoll.h"

#include <entt/entity/registry.hpp>

#include <vector>


namespace Scrapper {
// Runtime of a Ragdoll entity, owned by the Jolt module. Its parts carry PhysicsBody (ragdollPart).
struct RagdollInstance {
    JPH::Ref<JPH::Ragdoll> ragdoll; // null: creation failed
    bool inWorld = false;
    std::vector<float> masses; // per part
    float totalMass = 0.0f;
    SyncedPose synced;         // of the entity, which follows the root part
};

namespace RagdollBodies {
void CreatePending(entt::registry& registry, JoltWorld& jolt);
void Reconcile(entt::registry& registry);     // in the world unless the entity is Disabled
void TeleportMoved(entt::registry& registry, JoltWorld& jolt); // game code moved the entity: the parts go along
void ApplyCommands(entt::registry& registry, JoltWorld& jolt, bool includeImpulses);
void FollowRoots(entt::registry& registry, JoltWorld& jolt);  // entity <- root part; before the parts are read back
// The ragdoll as one rigid body: every part gets the same velocity change and spin around the common center of mass.
void AddWholeBodyVelocity(JoltWorld& jolt, const RagdollInstance& instance, JPH::Vec3Arg linear, JPH::Vec3Arg angular);
void AddWholeBodyImpulse(JoltWorld& jolt, const RagdollInstance& instance, JPH::Vec3Arg linear, JPH::Vec3Arg angular);
void OnDestroyed(entt::registry& registry, entt::entity entity);
}

}
