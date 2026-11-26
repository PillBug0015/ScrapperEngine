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
#include <Jolt/Physics/Constraints/SwingTwistConstraint.h>
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

// Idle, blended into walk by speed and into air off the ground; the motors chase it as hard as stability allows.
void DrivePose(const RagdollAsset& asset, RagdollInstance& instance, SpringCharacter& c, float speed, float stability, float deltaTime) {
    constexpr float AirBlendRate = 6.0f; // per second

    const RagdollAsset::Clip* idle = c.idleClip ? &asset.At(*c.idleClip) : nullptr;
    const RagdollAsset::Clip* walk = c.walkClip ? &asset.At(*c.walkClip) : nullptr;
    const RagdollAsset::Clip* air = c.airClip ? &asset.At(*c.airClip) : nullptr;
    const float walkBlend = walk != nullptr && c.walkClipSpeed > 0.0f ? Clamp(speed / c.walkClipSpeed, 0.0f, 1.0f) : 0.0f;
    c.clock += deltaTime;
    if (walkBlend > 0.0f) c.walkPhase += deltaTime * speed / c.walkClipSpeed; // stride follows the ground speed
    c.airBlend = Clamp(c.airBlend + (c.grounded ? -1.0f : 1.0f) * AirBlendRate * deltaTime, 0.0f, air != nullptr ? 1.0f : 0.0f);

    for (std::size_t i = 0; i < asset.parts.size(); ++i) {
        const int index = asset.settings->GetConstraintIndexForBodyIndex(static_cast<int>(i));
        JPH::TwoBodyConstraint* constraint = index >= 0 ? instance.ragdoll->GetConstraint(index) : nullptr;
        if (constraint == nullptr || constraint->GetSubType() != JPH::EConstraintSubType::SwingTwist) continue;

        const PartId part{ static_cast<std::uint16_t>(i) };
        Quaternion target = idle != nullptr ? idle->Sample(part, c.clock) : QuaternionIdentity();
        if (walkBlend > 0.0f) target = QuaternionSlerp(target, walk->Sample(part, c.walkPhase), walkBlend);
        if (c.airBlend > 0.0f) target = QuaternionSlerp(target, air->Sample(part, c.clock), c.airBlend);

        auto& joint = static_cast<JPH::SwingTwistConstraint&>(*constraint);
        joint.GetSwingMotorSettings().SetTorqueLimit(asset.motorTorque * stability);
        joint.GetTwistMotorSettings().SetTorqueLimit(asset.motorTorque * stability);
        joint.SetSwingMotorState(JPH::EMotorState::Position);
        joint.SetTwistMotorState(JPH::EMotorState::Position);
        joint.SetTargetOrientationBS(ToJolt(target));
    }
}
}

void StepCharacter(JoltWorld& jolt, const CharacterSpringScale& scale, const RagdollAsset& asset, RagdollInstance& instance,
                   SpringCharacter& c, StandingOn& standingOn, float deltaTime) {
    c.jumpTimer = std::max(0.0f, c.jumpTimer - deltaTime);
    const float stability = c.stability = UpdateStability(c, deltaTime);

    JPH::BodyInterface& bodies = jolt.Bodies();
    const JPH::BodyID core = instance.ragdoll->GetBodyID(c.core.value);
    const float centerHeight = CenterHeight(asset, c.core, c.hover);
    const JPH::Vec3 up = JPH::Vec3::sAxisY();
    const float gravity = -jolt.System().GetGravity().GetY();

    const JPH::Vec3 velocity = bodies.GetLinearVelocity(core);
    const JPH::Vec3 angularVelocity = bodies.GetAngularVelocity(core);
    const JPH::Quat rotation = bodies.GetRotation(core);

    const GroundProbe ground = ProbeGround(jolt, *instance.ragdoll, centerHeight, bodies.GetCenterOfMassPosition(core));
    c.grounded = ground.hit && c.jumpTimer <= 0.0f;
    if (c.grounded) {
        // Kept while airborne: a jump inside a moving tank still belongs to the tank.
        standingOn.entity = bodies.GetMotionType(ground.body) == JPH::EMotionType::Static
            ? entt::null : ToEntity(bodies.GetUserData(ground.body));
    }

    // Changes go to the whole body alike: the springs carry it, the motors only shape it.
    JPH::Vec3 deltaVelocity = JPH::Vec3::sZero();

    // 1. Leg: hold the core at standing height, carrying gravity while grounded.
    //    Weakened by stability, so a stunned character's knees give way.
    if (c.grounded) {
        const float offset = centerHeight - ground.distance;
        const float relativeVelocity = (velocity - ground.velocity).Dot(up);
        const float acceleration = stability * std::clamp(SpringAcceleration(c.leg, scale, offset, relativeVelocity, deltaTime) + gravity,
                                                          -MaxLegPull, gravity + MaxLegPush);
        deltaVelocity += up * (acceleration * deltaTime);

        // Newton's third law: our weight presses on what we stand on.
        if (bodies.GetMotionType(ground.body) == JPH::EMotionType::Dynamic) {
            bodies.AddImpulse(ground.body, -up * (acceleration * deltaTime * instance.totalMass), ground.point);
        }
    }

    // 2. Movement. The body floats, so there is no ground friction: standing still on a
    //    moving vehicle is the control matching the ground's velocity. A stunned one slides.
    const JPH::Vec3 input = ToJolt(c.moveInput);
    const JPH::Vec3 horizontal(velocity.GetX(), 0.0f, velocity.GetZ());
    const JPH::Vec3 groundHorizontal(ground.velocity.GetX(), 0.0f, ground.velocity.GetZ());
    if (c.grounded) {
        const JPH::Vec3 target = input * c.moveSpeed + groundHorizontal;
        deltaVelocity += MoveTowards(horizontal, target, stability * c.groundAcceleration * deltaTime) - horizontal;

        if (c.jumpRequested) {
            const float jumpVelocity = ground.velocity.GetY() + c.jumpSpeed;
            deltaVelocity += up * (stability * (jumpVelocity - velocity.GetY()));
            c.jumpTimer = JumpLegOffSeconds;
            c.grounded = false;
        }
    } else if (input.LengthSq() > 0.0f) {
        deltaVelocity += MoveTowards(horizontal, input * c.moveSpeed, stability * c.airAcceleration * deltaTime) - horizontal;
    }
    c.jumpRequested = false;

    // 3. Upright: one spring on all three axes towards the core's rest orientation, turned to facingYaw.
    //    It turns the whole body around its center of mass: on the core alone the limbs would outweigh it.
    JPH::Quat error = JPH::Quat::sRotation(up, c.facingYaw) * ToJolt(asset.At(c.core).rotation) * rotation.Conjugated();
    if (error.GetW() < 0.0f) error = -error; // shortest way round

    JPH::Vec3 axis;
    float angle = 0.0f;
    error.GetAxisAngle(axis, angle);

    JPH::Vec3 angularAcceleration = stability * SpringAcceleration(c.upright, scale, axis * angle, angularVelocity, deltaTime);
    const float length = angularAcceleration.Length();
    if (length > MaxAngularAcceleration) angularAcceleration *= MaxAngularAcceleration / length;
    RagdollBodies::AddWholeBodyVelocity(jolt, instance, deltaVelocity, angularAcceleration * deltaTime);

    // 4. Limbs.
    DrivePose(asset, instance, c, (horizontal - groundHorizontal).Length(), stability, deltaTime);
}

}
