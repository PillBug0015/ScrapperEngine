#include "Hands.h"
#include "GripSync.h"
#include "Scrapper/Jolt/Grip.h"
#include "Scrapper/Jolt/PhysicsQueries.h"
#include "Scrapper/Jolt/RagdollPose.h"
#include "Scrapper/Jolt/SpringCharacter.h"

using namespace Scrapper;

namespace {
constexpr float ReachedEnough = 0.6f;    // aim weight before the hand starts feeling for a hold
constexpr float TouchRadius = 0.12f;     // around the hand's tip
constexpr float MinGripStability = 0.3f; // a stunned body lets go
constexpr float SlipDistance = 0.4f;     // stretched this far, the hold took more than the grip has

// A hand that slipped stays open until its button is let go, instead of grabbing straight back.
struct Slipped {};

Vector3 ToWorld(const entt::registry& registry, entt::entity entity, Vector3 local) {
    const WorldTransform pose = Transforms::ComputeWorld(registry, entity);
    return Vector3Add(pose.position, Vector3RotateByQuaternion(local, pose.rotation));
}

Vector3 ToLocal(const entt::registry& registry, entt::entity entity, Vector3 point) {
    const WorldTransform pose = Transforms::ComputeWorld(registry, entity);
    return Vector3RotateByQuaternion(Vector3Subtract(point, pose.position), QuaternionInvert(pose.rotation));
}

void Release(World& world, entt::entity hand) {
    if (!world.Registry().all_of<Grip>(hand)) return;
    world.Registry().remove<Grip>(hand);
    SendGrip(world, hand);
}

void Grab(World& world, GameObject body, PartId arm, entt::entity hand) {
    entt::registry& registry = world.Registry();
    const Vector3 tip = body.Get<Ragdoll>().asset->At(arm).tip;
    const std::optional<Touch> touch = FindTouch(world, ToWorld(registry, hand, tip), TouchRadius, body.Entity());
    if (!touch) return;
    registry.emplace<Grip>(hand, Grip{ touch->entity, tip, ToLocal(registry, touch->entity, touch->point) });
    SendGrip(world, hand);
}
}

void UpdateHand(World& world, GameObject body, PartId arm, bool held, Vector3 reachPoint) {
    entt::registry& registry = world.Registry();
    PoseAim* aim = body.Get<RagdollPose>().Aim(arm);
    if (aim == nullptr) return;
    const entt::entity hand = Ragdolls::PartEntity(registry, body.Entity(), arm);

    if (!held || body.Get<SpringCharacter>().stability < MinGripStability) {
        aim->target = 0.0f;
        registry.remove<Slipped>(hand);
        Release(world, hand);
        return;
    }
    aim->target = 1.0f;
    if (const Grip* grip = registry.try_get<Grip>(hand)) {
        const Vector3 hold = ToWorld(registry, grip->target, grip->targetAnchor);
        aim->point = hold; // the arm keeps pointing at the hold
        if (Vector3Distance(ToWorld(registry, hand, grip->anchor), hold) < SlipDistance) return;
        registry.emplace<Slipped>(hand);
        Release(world, hand);
        return;
    }
    aim->point = reachPoint;
    if (aim->weight >= ReachedEnough && !registry.all_of<Slipped>(hand)) Grab(world, body, arm, hand);
}
