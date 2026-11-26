#include "Scrapper/Jolt/JoltModule.h"
#include "Scrapper/Jolt/JoltConversion.h"
#include "Scrapper/Jolt/JoltWorld.h"
#include "Scrapper/Jolt/PhysicsBody.h"
#include "Scrapper/Jolt/SpringCharacter.h"
#include "Scrapper/Core/Authority.h"
#include "Scrapper/Core/Lifecycle.h"
#include "Scrapper/Core/Transform.h"
#include "Scrapper/Core/World.h"
#include "BodyEntity.h"
#include "CharacterStep.h"
#include "GripJoints.h"
#include "PoseDrive.h"
#include "RagdollBody.h"

#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyInterface.h>

#include <algorithm>
#include <cstdio>
#include <vector>


namespace Scrapper {
namespace {
JPH::BodyID CreateRigidBody(JoltWorld& jolt, const RigidBody& desc, const WorldTransform& pose, entt::entity entity) {
    JPH::BodyCreationSettings settings = desc.settings;
    settings.mPosition = ToJoltR(pose.position);
    settings.mRotation = ToJolt(pose.rotation);
    settings.mUserData = ToUserData(entity);
    const JPH::Body* body = jolt.Bodies().CreateBody(settings);
    return body != nullptr ? body->GetID() : JPH::BodyID();
}

void CreatePendingBodies(entt::registry& registry, JoltWorld& jolt) {
    std::vector<entt::entity> pending;
    for (entt::entity e : registry.view<RigidBody, LocalTransform>(entt::exclude<PhysicsBody>)) pending.push_back(e);

    for (entt::entity e : pending) {
        const RigidBody& desc = registry.get<RigidBody>(e);
        const JPH::BodyID id = CreateRigidBody(jolt, desc, Transforms::ComputeWorld(registry, e), e);
        if (id.IsInvalid()) {
            std::fprintf(stderr, "[Jolt] body limit reached\n");
            continue;
        }
        PhysicsBody& body = registry.emplace<PhysicsBody>(e);
        body.id = id;
        body.authoredMotion = desc.settings.mMotionType;
        body.motion = desc.settings.mMotionType;
        body.synced.Remember(registry.get<LocalTransform>(e));
    }
    RagdollBodies::CreatePending(registry, jolt);
}

void ReconcileBodies(entt::registry& registry, JoltWorld& jolt) {
    JPH::BodyInterface& bodies = jolt.Bodies();
    for (auto [e, body, local] : registry.view<PhysicsBody, LocalTransform>().each()) {
        const bool remote = registry.all_of<RemoteAuthority>(e) && body.authoredMotion != JPH::EMotionType::Static;
        const JPH::EMotionType wanted = remote ? JPH::EMotionType::Kinematic : body.authoredMotion;
        if (body.motion != wanted) {
            bodies.SetMotionType(body.id, wanted, JPH::EActivation::Activate);
            body.motion = wanted;
        }

        if (body.ragdollPart) continue; // its ragdoll goes in and out of the world as a whole
        const bool wantInWorld = !registry.all_of<Disabled>(e);
        if (wantInWorld && !body.inWorld) {
            const WorldTransform pose = Transforms::ComputeWorld(registry, e);
            bodies.SetPositionAndRotation(body.id, ToJoltR(pose.position), ToJolt(pose.rotation), JPH::EActivation::DontActivate);
            bodies.AddBody(body.id, body.motion == JPH::EMotionType::Static ? JPH::EActivation::DontActivate : JPH::EActivation::Activate);
            body.inWorld = true;
            body.synced.Remember(local);
        } else if (!wantInWorld && body.inWorld) {
            bodies.RemoveBody(body.id);
            body.inWorld = false;
        }
    }
    RagdollBodies::Reconcile(registry);
}

// Game code moved a dynamic or static body's LocalTransform: teleport the body there.
void TeleportMovedBodies(entt::registry& registry, JoltWorld& jolt) {
    for (auto [e, body, local, world] : registry.view<PhysicsBody, LocalTransform, WorldTransform>().each()) {
        if (!body.inWorld || body.motion == JPH::EMotionType::Kinematic || body.synced.Matches(local)) continue;
        jolt.Bodies().SetPositionAndRotation(body.id, ToJoltR(world.position), ToJolt(world.rotation), JPH::EActivation::Activate);
        body.synced.Remember(local);
    }
    RagdollBodies::TeleportMoved(registry, jolt);
}

// A kinematic body follows its WorldTransform: from where Jolt has it now to there, over this frame's steps.
struct KinematicMove {
    JPH::BodyID id;
    JPH::Vec3 fromPosition, toPosition;
    JPH::Quat fromRotation, toRotation;
};

std::vector<KinematicMove> PlanKinematicMoves(entt::registry& registry, JPH::BodyInterface& bodies) {
    std::vector<KinematicMove> moves;
    for (auto [e, body, world] : registry.view<PhysicsBody, WorldTransform>().each()) {
        if (!body.inWorld || body.motion != JPH::EMotionType::Kinematic) continue;
        JPH::RVec3 position;
        JPH::Quat rotation;
        bodies.GetPositionAndRotation(body.id, position, rotation);
        moves.push_back({ body.id, JPH::Vec3(position), ToJolt(world.position), rotation, ToJolt(world.rotation) });
    }
    return moves;
}

bool IsSimulated(const PhysicsBody& body) {
    return body.inWorld && body.motion == JPH::EMotionType::Dynamic;
}

void StepCharacters(entt::registry& registry, JoltWorld& jolt, const CharacterSpringScale& scale) {
    for (auto [e, character, ragdoll, instance] : registry.view<SpringCharacter, Ragdoll, RagdollInstance>().each()) {
        if (!RagdollBodies::IsSimulatedHere(registry, e, instance)) continue;
        const float groundSpeed = StepCharacter(jolt, scale, *ragdoll.asset, instance, character, registry.get_or_emplace<StandingOn>(e),
                                                JoltWorld::FixedDeltaTime);
        if (RagdollPose* pose = registry.try_get<RagdollPose>(e)) FeedPose(*pose, *ragdoll.asset, character, groundSpeed);
    }
}

void DrivePoses(entt::registry& registry, JoltWorld& jolt) {
    for (auto [e, ragdoll, instance, pose] : registry.view<Ragdoll, RagdollInstance, RagdollPose>().each()) {
        if (!RagdollBodies::IsSimulatedHere(registry, e, instance)) continue;
        DrivePose(jolt.Bodies(), *ragdoll.asset, instance, pose, JoltWorld::FixedDeltaTime);
    }
}

void Simulate(World& world, float deltaTime) {
    entt::registry& registry = world.Registry();
    JoltWorld& jolt = world.Resource<JoltWorld>();
    JPH::BodyInterface& bodies = jolt.Bodies();

    CreatePendingBodies(registry, jolt);
    ReconcileBodies(registry, jolt);
    TeleportMovedBodies(registry, jolt);
    GripJoints::Sync(registry, jolt);

    for (auto [e, body, commands] : registry.view<PhysicsBody, PhysicsCommands>().each()) {
        if (!IsSimulated(body)) continue;
        if (commands.overrideLinearVelocity) bodies.SetLinearVelocity(body.id, ToJolt(commands.linearVelocity));
        if (Vector3LengthSqr(commands.impulse) > 0.0f) bodies.AddImpulse(body.id, ToJolt(commands.impulse));
        if (Vector3LengthSqr(commands.angularImpulse) > 0.0f) bodies.AddAngularImpulse(body.id, ToJolt(commands.angularImpulse));
    }

    jolt.accumulator = std::min(jolt.accumulator + deltaTime, JoltWorld::FixedDeltaTime * JoltWorld::MaxStepsPerFrame);
    const int steps = static_cast<int>(jolt.accumulator / JoltWorld::FixedDeltaTime);
    const std::vector<KinematicMove> kinematic = PlanKinematicMoves(registry, bodies);
    for (int step = 0; step < steps; ++step) {
        // Spread over this frame's steps: all in the first would leave a riding character a 2v/0 floor.
        const float progress = static_cast<float>(step + 1) / static_cast<float>(steps);
        for (const KinematicMove& move : kinematic) {
            bodies.MoveKinematic(move.id, JPH::RVec3(move.fromPosition + (move.toPosition - move.fromPosition) * progress),
                                 move.fromRotation.SLERP(move.toRotation, progress), JoltWorld::FixedDeltaTime);
        }
        for (auto [e, body, commands] : registry.view<PhysicsBody, PhysicsCommands>().each()) {
            if (!IsSimulated(body)) continue;
            if (Vector3LengthSqr(commands.force) > 0.0f) bodies.AddForce(body.id, ToJolt(commands.force));
            if (Vector3LengthSqr(commands.torque) > 0.0f) bodies.AddTorque(body.id, ToJolt(commands.torque));
        }
        RagdollBodies::ApplyCommands(registry, jolt, step == 0); // impulses once per frame, forces every step
        StepCharacters(registry, jolt, world.Resource<CharacterSpringScale>());
        DrivePoses(registry, jolt);

        jolt.Update(JoltWorld::FixedDeltaTime);
        jolt.accumulator -= JoltWorld::FixedDeltaTime;
    }
    const bool stepped = steps > 0;

    for (auto [e, commands] : registry.view<PhysicsCommands>().each()) commands = {};
    if (stepped) RagdollBodies::FollowRoots(registry, jolt);

    for (auto [e, body] : registry.view<PhysicsBody>().each()) {
        if (!body.inWorld) continue;
        body.linearVelocity = ToRaylib(bodies.GetLinearVelocity(body.id));
        body.angularVelocity = ToRaylib(bodies.GetAngularVelocity(body.id));

        if (stepped && body.motion == JPH::EMotionType::Dynamic && bodies.IsActive(body.id)) {
            JPH::RVec3 position;
            JPH::Quat rotation;
            bodies.GetPositionAndRotation(body.id, position, rotation);
            Transforms::SetWorldPose(registry, e, ToRaylib(position), ToRaylib(rotation));
            body.synced.Remember(registry.get<LocalTransform>(e));
        }
    }
}

void OnBodyDestroyed(entt::registry& registry, entt::entity entity) {
    JoltWorld* jolt = registry.ctx().find<JoltWorld>();
    const PhysicsBody& body = registry.get<PhysicsBody>(entity);
    if (jolt == nullptr || body.id.IsInvalid() || body.ragdollPart) return;

    if (body.inWorld) jolt->Bodies().RemoveBody(body.id);
    jolt->Bodies().DestroyBody(body.id);
}
}

void JoltModule::Install(World& world) {
    world.AddResource<JoltWorld>();
    world.AddResource<CharacterSpringScale>();
    world.Registry().on_destroy<PhysicsBody>().connect<&OnBodyDestroyed>();
    world.Registry().on_destroy<RagdollInstance>().connect<&RagdollBodies::OnDestroyed>();
    world.Registry().on_destroy<GripJoint>().connect<&GripJoints::OnDestroyed>();
    world.GetPipeline().Add(JoltSystem::Simulate, &Simulate, Placement::InPhase(Phase::Simulate));
}

void JoltModule::Uninstall(World& world) {
    world.GetPipeline().Remove(JoltSystem::Simulate);
    // Every joint and body goes while the physics system still exists.
    world.Registry().clear<GripJoint>();
    world.Registry().clear<PhysicsBody>();
    world.Registry().clear<RagdollInstance>();
    world.Registry().on_destroy<PhysicsBody>().disconnect<&OnBodyDestroyed>();
    world.Registry().on_destroy<RagdollInstance>().disconnect<&RagdollBodies::OnDestroyed>();
    world.Registry().on_destroy<GripJoint>().disconnect<&GripJoints::OnDestroyed>();
    world.RemoveResource<CharacterSpringScale>();
    world.RemoveResource<JoltWorld>();
}

}
