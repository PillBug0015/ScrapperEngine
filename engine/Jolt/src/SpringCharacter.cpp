#include "CharacterStep.h"
#include "BodyEntity.h"
#include "Scrapper/Jolt/JoltConversion.h"

#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyFilter.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/NarrowPhaseQuery.h>
#include <Jolt/Physics/Collision/ShapeCast.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/PhysicsSystem.h>

#include <algorithm>


namespace Scrapper {
namespace Characters {
void AddStun(SpringCharacter& character, float seconds, float factor) {
    if (seconds <= 0.0f) return;
    character.stuns.push_back({ Clamp(factor, 0.0f, 1.0f), seconds, 0.0f });
}

void Knockback(SpringCharacter& character, PhysicsCommands& commands, Vector3 impulse, float stunSeconds) {
    constexpr float TipOver = 0.25f; // angular impulse per unit of horizontal impulse

    AddStun(character, stunSeconds, 0.0f);
    commands.impulse = Vector3Add(commands.impulse, impulse);
    commands.angularImpulse = Vector3Add(commands.angularImpulse,
        Vector3Scale(Vector3CrossProduct({ 0.0f, 1.0f, 0.0f }, { impulse.x, 0.0f, impulse.z }), TipOver));
}
}

namespace {
constexpr float LegReach = 0.4f;          // how far below standing height the leg still finds ground
constexpr float JumpLegOffSeconds = 0.25f;
constexpr float MaxLegPush = 40.0f;       // m/s^2 on top of gravity: limits launches after hard landings
constexpr float MaxLegPull = 15.0f;       // m/s^2 downward: sticks to the ground over small drops
constexpr float MaxAngularAcceleration = 120.0f;
constexpr float ProbeRadius = 0.1f;

struct GroundProbe {
    bool hit = false;
    float distance = 0.0f; // from the center of mass down to the ground
    JPH::BodyID body;
    JPH::RVec3 point;
    JPH::Vec3 velocity = JPH::Vec3::sZero();
};

// Stays low for a while, then eases back to 1: knocked down, then getting up.
float StunValue(const Stun& stun) {
    const float t = Clamp(stun.elapsed / stun.duration, 0.0f, 1.0f);
    const float u = t * t;
    return stun.factor + (1.0f - stun.factor) * u * u * (3.0f - 2.0f * u);
}

float UpdateStability(SpringCharacter& c, float deltaTime) {
    float stability = 1.0f;
    for (Stun& stun : c.stuns) {
        stun.elapsed += deltaTime;
        stability = std::min(stability, StunValue(stun));
    }
    c.stuns.erase(std::remove_if(c.stuns.begin(), c.stuns.end(), [](const Stun& stun) { return stun.elapsed >= stun.duration; }),
                  c.stuns.end());
    return stability;
}

// a = w^2 * x - 2 * zeta * w * v, with w and the damping term capped so a 60 Hz step can't blow up.
template <typename T>
T SpringAcceleration(const SpringParams& spring, const CharacterSpringScale& scale, T offset, T velocity, float deltaTime) {
    const float omega = std::min(2.0f * JPH::JPH_PI * spring.frequency * scale.stiffness, 1.0f / deltaTime);
    const float damping = std::min(2.0f * spring.dampingRatio * scale.damping * omega, 1.0f / deltaTime);
    return offset * (omega * omega) - velocity * damping;
}

JPH::Vec3 MoveTowards(JPH::Vec3Arg current, JPH::Vec3Arg target, float maxDelta) {
    const JPH::Vec3 delta = target - current;
    const float length = delta.Length();
    return length <= maxDelta || length < 1e-6f ? target : current + delta * (maxDelta / length);
}

class IgnoreBodies final : public JPH::BodyFilter {
public:
    explicit IgnoreBodies(const JPH::Array<JPH::BodyID>& ids) : ids(ids) {}
    bool ShouldCollide(const JPH::BodyID& id) const override { return std::find(ids.begin(), ids.end(), id) == ids.end(); }

private:
    const JPH::Array<JPH::BodyID>& ids;
};

// A small sphere instead of a ray, so the leg doesn't slip through gaps at edges. Ignores the own limbs.
GroundProbe ProbeGround(JoltWorld& jolt, const JPH::Ragdoll& ragdoll, float centerHeight, JPH::RVec3Arg centerOfMass) {
    const float castLength = centerHeight + LegReach - ProbeRadius;

    JPH::SphereShape probe(ProbeRadius);
    probe.SetEmbedded();
    const JPH::RShapeCast cast(&probe, JPH::Vec3::sOne(), JPH::RMat44::sTranslation(centerOfMass), JPH::Vec3(0.0f, -castLength, 0.0f));
    JPH::ClosestHitCollisionCollector<JPH::CastShapeCollector> collector;
    const IgnoreBodies ignoreSelf(ragdoll.GetBodyIDs());
    jolt.System().GetNarrowPhaseQuery().CastShape(cast, JPH::ShapeCastSettings(), centerOfMass, collector, {}, {}, ignoreSelf);

    GroundProbe ground;
    if (!collector.HadHit()) return ground;

    ground.hit = true;
    ground.distance = collector.mHit.mFraction * castLength + ProbeRadius;
    ground.body = collector.mHit.mBodyID2;
    ground.point = centerOfMass + collector.mHit.mContactPointOn2;
    ground.velocity = jolt.Bodies().GetPointVelocity(ground.body, ground.point);
    return ground;
}

// How high the core's center of mass rides when standing: as at rest, plus the hover.
float CenterHeight(const RagdollAsset& asset, PartId core, float hover) {
    const RagdollAsset::Part& rest = asset.At(core);
    const JPH::Vec3 centerOfMass = ToJolt(rest.rotation) * asset.settings->mParts[core.value].GetShape()->GetCenterOfMass();
    return rest.position.y + centerOfMass.GetY() + hover;
}

// What a step reads from the core part, before it changes anything.
struct CoreState {
    JPH::Vec3 velocity;
    JPH::Vec3 angularVelocity;
    JPH::Quat rotation;
    JPH::RVec3 centerOfMass;
};

CoreState ReadCore(JPH::BodyInterface& bodies, JPH::BodyID core) {
    return { bodies.GetLinearVelocity(core), bodies.GetAngularVelocity(core), bodies.GetRotation(core), bodies.GetCenterOfMassPosition(core) };
}

JPH::Vec3 Horizontal(JPH::Vec3Arg v) { return JPH::Vec3(v.GetX(), 0.0f, v.GetZ()); }

// Grounded unless just jumped. What it stands on is kept while airborne: a jump inside a moving tank
// still belongs to the tank.
void StandOn(SpringCharacter& c, const GroundProbe& ground, StandingOn& standingOn, JPH::BodyInterface& bodies) {
    c.grounded = ground.hit && c.jumpTimer <= 0.0f;
    if (!c.grounded) return;
    standingOn.entity = bodies.GetMotionType(ground.body) == JPH::EMotionType::Static ? entt::null : ToEntity(bodies.GetUserData(ground.body));
}

// Holds the core at standing height, carrying gravity while grounded. Weakened by stability, so a stunned
// character's knees give way.
float LegAcceleration(const SpringCharacter& c, const CharacterSpringScale& scale, const GroundProbe& ground, const CoreState& core,
                      float centerHeight, float gravity, float deltaTime) {
    if (!c.grounded) return 0.0f;
    const float offset = centerHeight - ground.distance;
    const float relativeVelocity = core.velocity.GetY() - ground.velocity.GetY();
    return c.stability * std::clamp(SpringAcceleration(c.leg, scale, offset, relativeVelocity, deltaTime) + gravity,
                                    -MaxLegPull, gravity + MaxLegPush);
}

// Newton's third law: our weight presses on what we stand on.
void PressGround(JPH::BodyInterface& bodies, const GroundProbe& ground, float liftImpulse) {
    if (liftImpulse == 0.0f || bodies.GetMotionType(ground.body) != JPH::EMotionType::Dynamic) return;
    bodies.AddImpulse(ground.body, JPH::Vec3(0.0f, -liftImpulse, 0.0f), ground.point);
}

// The body floats, so there is no ground friction: standing still on a moving vehicle is the control
// matching the ground's velocity. A stunned one slides.
JPH::Vec3 Steering(const SpringCharacter& c, const GroundProbe& ground, const CoreState& core, float deltaTime) {
    const JPH::Vec3 input = ToJolt(c.moveInput);
    const JPH::Vec3 horizontal = Horizontal(core.velocity);
    if (c.grounded) {
        const JPH::Vec3 target = input * c.moveSpeed + Horizontal(ground.velocity);
        return MoveTowards(horizontal, target, c.stability * c.groundAcceleration * deltaTime) - horizontal;
    }
    if (input.LengthSq() == 0.0f) return JPH::Vec3::sZero();
    return MoveTowards(horizontal, input * c.moveSpeed, c.stability * c.airAcceleration * deltaTime) - horizontal;
}

// Consumes the request; only a grounded character leaves the ground, and the leg lets go for a moment.
JPH::Vec3 Jump(SpringCharacter& c, const GroundProbe& ground, const CoreState& core) {
    const bool jumps = c.jumpRequested && c.grounded;
    c.jumpRequested = false;
    if (!jumps) return JPH::Vec3::sZero();
    c.jumpTimer = JumpLegOffSeconds;
    c.grounded = false;
    const float jumpVelocity = ground.velocity.GetY() + c.jumpSpeed;
    return JPH::Vec3(0.0f, c.stability * (jumpVelocity - core.velocity.GetY()), 0.0f);
}

// One spring on all three axes towards the core's rest orientation, turned to facingYaw.
JPH::Vec3 UprightAcceleration(const SpringCharacter& c, const CharacterSpringScale& scale, Quaternion rest, const CoreState& core,
                              float deltaTime) {
    JPH::Quat error = JPH::Quat::sRotation(JPH::Vec3::sAxisY(), c.facingYaw) * ToJolt(rest) * core.rotation.Conjugated();
    if (error.GetW() < 0.0f) error = -error; // shortest way round

    JPH::Vec3 axis;
    float angle = 0.0f;
    error.GetAxisAngle(axis, angle);
    const JPH::Vec3 acceleration = c.stability * SpringAcceleration(c.upright, scale, axis * angle, core.angularVelocity, deltaTime);
    const float length = acceleration.Length();
    return length > MaxAngularAcceleration ? acceleration * (MaxAngularAcceleration / length) : acceleration;
}
}

// Everything goes to the whole body alike: the springs carry it, the motors only shape it.
float StepCharacter(JoltWorld& jolt, const CharacterSpringScale& scale, const RagdollAsset& asset, RagdollInstance& instance,
                    SpringCharacter& c, StandingOn& standingOn, float deltaTime) {
    c.jumpTimer = std::max(0.0f, c.jumpTimer - deltaTime);
    c.stability = UpdateStability(c, deltaTime);

    JPH::BodyInterface& bodies = jolt.Bodies();
    const CoreState core = ReadCore(bodies, instance.ragdoll->GetBodyID(c.core.value));
    const float centerHeight = CenterHeight(asset, c.core, c.hover);
    const GroundProbe ground = ProbeGround(jolt, *instance.ragdoll, centerHeight, core.centerOfMass);
    StandOn(c, ground, standingOn, bodies);

    const float gravity = -jolt.System().GetGravity().GetY();
    const float lift = LegAcceleration(c, scale, ground, core, centerHeight, gravity, deltaTime) * deltaTime;
    PressGround(bodies, ground, lift * instance.totalMass);
    const JPH::Vec3 steering = Steering(c, ground, core, deltaTime);
    const JPH::Vec3 jump = Jump(c, ground, core); // after steering: it leaves the ground
    const JPH::Vec3 spin = UprightAcceleration(c, scale, asset.At(c.core).rotation, core, deltaTime) * deltaTime;

    RagdollBodies::AddWholeBodyVelocity(jolt, instance, JPH::Vec3(0.0f, lift, 0.0f) + steering + jump, spin);
    return (Horizontal(core.velocity) - Horizontal(ground.velocity)).Length();
}

void FeedPose(RagdollPose& pose, const RagdollAsset& asset, const SpringCharacter& character, float groundSpeed) {
    for (PoseLayer& layer : pose.layers) {
        switch (layer.driver) {
            case PoseDriver::Manual:
                break;
            case PoseDriver::Always:
                layer.target = 1.0f;
                layer.pace = 1.0f;
                break;
            case PoseDriver::Moving: {
                const RagdollAsset::Clip& clip = asset.At(layer.clip);
                const float strideSpeed = clip.duration > 0.0f ? clip.stride / clip.duration : 0.0f;
                layer.pace = strideSpeed > 0.0f ? groundSpeed / strideSpeed : 0.0f; // feet keep up with the ground
                layer.target = Clamp(layer.pace, 0.0f, 1.0f);
                break;
            }
            case PoseDriver::Airborne:
                layer.target = character.grounded ? 0.0f : 1.0f;
                layer.pace = 1.0f;
                break;
        }
    }
    pose.strength = character.stability;
}

}
