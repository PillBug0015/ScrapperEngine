#include "RagdollBody.h"
#include "BodyEntity.h"
#include "Scrapper/Core/Authority.h"
#include "Scrapper/Core/Lifecycle.h"
#include "Scrapper/Core/Name.h"
#include "Scrapper/Core/Parts.h"
#include "Scrapper/Jolt/JoltConversion.h"

#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Body/BodyLock.h>
#include <Jolt/Physics/PhysicsSystem.h>

#include <cstdio>


namespace Scrapper {
void Ragdolls::Add(entt::registry& registry, entt::entity entity, std::shared_ptr<const RagdollAsset> asset) {
    Parts parts;
    for (const RagdollAsset::Part& rest : asset->parts) {
        const entt::entity part = registry.create();
        registry.emplace<LocalTransform>(part, LocalTransform{ rest.position, rest.rotation });
        registry.emplace<Name>(part, rest.name);
        Transforms::SetParent(registry, part, entity);
        parts.entities.push_back(part);
    }
    registry.emplace_or_replace<Parts>(entity, std::move(parts));
    registry.emplace_or_replace<Ragdoll>(entity, std::move(asset));
}

entt::entity Ragdolls::PartEntity(const entt::registry& registry, entt::entity entity, PartId part) {
    return registry.get<Parts>(entity).entities[part.value]; // Add creates them in PartId order
}

namespace RagdollBodies {
namespace {
void SetInWorld(entt::registry& registry, entt::entity entity, RagdollInstance& instance, bool inWorld) {
    if (inWorld) instance.ragdoll->AddToPhysicsSystem(JPH::EActivation::Activate);
    else instance.ragdoll->RemoveFromPhysicsSystem();
    instance.inWorld = inWorld;
    for (entt::entity part : registry.get<Parts>(entity).entities) registry.get<PhysicsBody>(part).inWorld = inWorld;
}

void Create(entt::registry& registry, JoltWorld& jolt, entt::entity entity) {
    const RagdollAsset& asset = *registry.get<Ragdoll>(entity).asset;
    const std::vector<entt::entity>& parts = registry.get<Parts>(entity).entities;
    RagdollInstance& instance = registry.emplace<RagdollInstance>(entity);

    // Its own group id: the parts don't collide with each other, but do with other ragdolls.
    instance.ragdoll = asset.settings->CreateRagdoll(entt::to_integral(entity), 0, &jolt.System());
    if (instance.ragdoll == nullptr || parts.size() != instance.ragdoll->GetBodyCount()) {
        std::fprintf(stderr, "[Jolt] ragdoll not created (body limit, or its Parts were changed)\n");
        instance.ragdoll = nullptr;
        return;
    }

    JPH::BodyInterface& bodies = jolt.Bodies();
    for (std::size_t i = 0; i < parts.size(); ++i) {
        const JPH::BodyID id = instance.ragdoll->GetBodyID(static_cast<int>(i));
        const WorldTransform pose = Transforms::ComputeWorld(registry, parts[i]);
        bodies.SetUserData(id, ToUserData(parts[i]));
        bodies.SetPositionAndRotation(id, ToJoltR(pose.position), ToJolt(pose.rotation), JPH::EActivation::DontActivate);

        PhysicsBody& body = registry.emplace<PhysicsBody>(parts[i]);
        body.id = id;
        body.ragdollPart = true;
        body.synced.Remember(registry.get<LocalTransform>(parts[i]));

        JPH::BodyLockRead lock(jolt.System().GetBodyLockInterface(), id);
        instance.masses.push_back(1.0f / lock.GetBody().GetMotionProperties()->GetInverseMass());
        instance.totalMass += instance.masses.back();
    }
    instance.synced.Remember(registry.get<LocalTransform>(entity));
    if (!registry.all_of<Disabled>(entity)) SetInWorld(registry, entity, instance, true);
}
}

bool IsSimulatedHere(const entt::registry& registry, entt::entity entity, const RagdollInstance& instance) {
    return instance.ragdoll != nullptr && instance.inWorld && !registry.all_of<RemoteAuthority>(entity);
}

void CreatePending(entt::registry& registry, JoltWorld& jolt) {
    std::vector<entt::entity> pending;
    for (entt::entity e : registry.view<Ragdoll, Parts, LocalTransform>(entt::exclude<RagdollInstance>)) pending.push_back(e);
    for (entt::entity e : pending) Create(registry, jolt, e);
}

void Reconcile(entt::registry& registry) {
    for (auto [entity, instance] : registry.view<RagdollInstance>().each()) {
        const bool wantInWorld = !registry.all_of<Disabled>(entity);
        if (instance.ragdoll != nullptr && wantInWorld != instance.inWorld) SetInWorld(registry, entity, instance, wantInWorld);
    }
}

void TeleportMoved(entt::registry& registry, JoltWorld& jolt) {
    for (auto [entity, instance, local] : registry.view<RagdollInstance, LocalTransform>().each()) {
        if (instance.ragdoll == nullptr || instance.synced.Matches(local)) continue;
        instance.synced.Remember(local);
        if (registry.all_of<RemoteAuthority>(entity)) continue; // kinematic parts follow on their own

        for (entt::entity part : registry.get<Parts>(entity).entities) {
            const WorldTransform pose = Transforms::ComputeWorld(registry, part);
            jolt.Bodies().SetPositionAndRotation(registry.get<PhysicsBody>(part).id, ToJoltR(pose.position), ToJolt(pose.rotation),
                                                 JPH::EActivation::Activate);
        }
    }
}

namespace {
JPH::Vec3 CenterOfMass(JPH::BodyInterface& bodies, const RagdollInstance& instance) {
    const JPH::Array<JPH::BodyID>& ids = instance.ragdoll->GetBodyIDs();
    JPH::Vec3 center = JPH::Vec3::sZero();
    for (std::size_t i = 0; i < ids.size(); ++i) center += JPH::Vec3(bodies.GetCenterOfMassPosition(ids[i])) * instance.masses[i];
    return center / instance.totalMass;
}
}

void AddWholeBodyVelocity(JoltWorld& jolt, const RagdollInstance& instance, JPH::Vec3Arg linear, JPH::Vec3Arg angular) {
    JPH::BodyInterface& bodies = jolt.Bodies();
    const JPH::Vec3 center = CenterOfMass(bodies, instance);
    for (const JPH::BodyID id : instance.ragdoll->GetBodyIDs()) {
        const JPH::Vec3 r = JPH::Vec3(bodies.GetCenterOfMassPosition(id)) - center;
        bodies.SetLinearAndAngularVelocity(id, bodies.GetLinearVelocity(id) + linear + angular.Cross(r),
                                           bodies.GetAngularVelocity(id) + angular);
    }
}

void AddWholeBodyImpulse(JoltWorld& jolt, const RagdollInstance& instance, JPH::Vec3Arg linear, JPH::Vec3Arg angular) {
    // Inertia of the parts as point masses around the spin axis.
    JPH::Vec3 spin = JPH::Vec3::sZero();
    if (angular.LengthSq() > 0.0f) {
        JPH::BodyInterface& bodies = jolt.Bodies();
        const JPH::Array<JPH::BodyID>& ids = instance.ragdoll->GetBodyIDs();
        const JPH::Vec3 center = CenterOfMass(bodies, instance);
        const JPH::Vec3 axis = angular.Normalized();
        float inertia = 1e-3f;
        for (std::size_t i = 0; i < ids.size(); ++i) {
            const JPH::Vec3 r = JPH::Vec3(bodies.GetCenterOfMassPosition(ids[i])) - center;
            inertia += instance.masses[i] * (r.LengthSq() - JPH::Square(r.Dot(axis)));
        }
        spin = angular / inertia;
    }
    AddWholeBodyVelocity(jolt, instance, linear / instance.totalMass, spin);
}

void ApplyCommands(entt::registry& registry, JoltWorld& jolt, bool includeImpulses) {
    for (auto [entity, instance, commands] : registry.view<RagdollInstance, PhysicsCommands>().each()) {
        if (!IsSimulatedHere(registry, entity, instance)) continue;
        JPH::Vec3 linear = ToJolt(commands.force) * JoltWorld::FixedDeltaTime;
        JPH::Vec3 angular = ToJolt(commands.torque) * JoltWorld::FixedDeltaTime;
        if (includeImpulses) {
            if (commands.overrideLinearVelocity) instance.ragdoll->SetLinearVelocity(ToJolt(commands.linearVelocity));
            linear += ToJolt(commands.impulse);
            angular += ToJolt(commands.angularImpulse);
        }
        if (linear.LengthSq() > 0.0f || angular.LengthSq() > 0.0f) AddWholeBodyImpulse(jolt, instance, linear, angular);
    }
}

void FollowRoots(entt::registry& registry, JoltWorld& jolt) {
    for (auto [entity, instance] : registry.view<RagdollInstance>().each()) {
        if (!IsSimulatedHere(registry, entity, instance)) continue;
        JPH::RVec3 position;
        JPH::Quat rotation;
        jolt.Bodies().GetPositionAndRotation(instance.ragdoll->GetBodyID(0), position, rotation);
        Transforms::SetWorldPose(registry, entity, ToRaylib(JPH::Vec3(position)), ToRaylib(rotation));
        instance.synced.Remember(registry.get<LocalTransform>(entity));
    }
}

void OnDestroyed(entt::registry& registry, entt::entity entity) {
    RagdollInstance& instance = registry.get<RagdollInstance>(entity);
    if (instance.ragdoll != nullptr && instance.inWorld) instance.ragdoll->RemoveFromPhysicsSystem();
}
}

}
