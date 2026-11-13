#include "Physics/CharacterBody.h"
#include "Physics/JoltConversion.h"
#include "Physics/PhysicsWorld.h"

#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyFilter.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/NarrowPhaseQuery.h>
#include <Jolt/Physics/Collision/ShapeCast.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/PhysicsSystem.h>

#include <raymath.h>

#include <algorithm>
#include <cmath>


namespace ScrapperEngine {
namespace {
constexpr float LegReach = 0.4f;          // how far below standing height the leg still finds ground
constexpr float JumpLegOffSeconds = 0.25f;
constexpr float MaxLegPush = 40.0f;       // m/s^2 on top of gravity: limits launches after hard landings
constexpr float MaxLegPull = 15.0f;       // m/s^2 downward: sticks to the ground over small drops
constexpr float MaxAngularAcceleration = 120.0f; // rad/s^2

JPH::BodyInterface& Bodies() {
    return PhysicsWorld::Instance().GetBodyInterface();
}

// a = w^2 * x - 2 * zeta * w * v, with w and the damping term capped so a 60 Hz step can't blow up.
template <typename T>
T SpringAcceleration(const SpringParams& spring, T offset, T velocity, float deltaTime) {
    const float omega = std::min(2.0f * JPH::JPH_PI * spring.frequency * CharacterBody::globalStiffness, 1.0f / deltaTime);
    const float damping = std::min(2.0f * spring.dampingRatio * CharacterBody::globalDamping * omega, 1.0f / deltaTime);
    return offset * (omega * omega) - velocity * damping;
}

JPH::Vec3 MoveTowards(JPH::Vec3Arg current, JPH::Vec3Arg target, float maxDelta) {
    const JPH::Vec3 delta = target - current;
    const float length = delta.Length();
    return length <= maxDelta || length < 1e-6f ? target : current + delta * (maxDelta / length);
}
}

CharacterBody::CharacterBody(float height, float radius, float mass, float legLength)
    : PhysicsBody(JPH::EMotionType::Dynamic), height(height), radius(radius), mass(mass), legLength(legLength) {}

JPH::BodyID CharacterBody::CreateBody(Vector3 position, Quaternion rotation) {
    // The capsule starts above the feet; the gap below it is the leg the spring holds open.
    const float capsuleLength = height - legLength;
    const float halfCylinder = std::max(0.01f, capsuleLength * 0.5f - radius);
    centerHeight = legLength + capsuleLength * 0.5f;

    JPH::RotatedTranslatedShapeSettings shape(JPH::Vec3(0.0f, centerHeight, 0.0f), JPH::Quat::sIdentity(),
                                              new JPH::CapsuleShape(halfCylinder, radius));

    JPH::BodyCreationSettings settings(shape.Create().Get(), ToJoltR(position), ToJolt(rotation),
                                       JPH::EMotionType::Dynamic, PhysicsLayers::Moving);
    settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
    settings.mMassPropertiesOverride.mMass = mass;
    settings.mAllowSleeping = false; // the springs work every step
    settings.mFriction = 0.6f;
    settings.mAngularDamping = 0.2f;

    probeShape = new JPH::SphereShape(radius * 0.5f);
    facingYaw = 2.0f * std::atan2(rotation.y, rotation.w);

    const JPH::Body* body = Bodies().CreateBody(settings);
    return body != nullptr ? body->GetID() : JPH::BodyID();
}

void CharacterBody::OverrideTuning(const CharacterTuning& tuning, float seconds) {
    overrideTuning = tuning;
    overrideTimer = seconds;
}

void CharacterBody::Knockback(Vector3 impulse, float limpSeconds) {
    constexpr float TipOver = 0.25f; // angular impulse per unit of horizontal impulse

    OverrideTuning(CharacterTunings::Limp, limpSeconds);
    AddImpulse(impulse);
    AddAngularImpulse(Vector3Scale(Vector3CrossProduct({ 0.0f, 1.0f, 0.0f }, { impulse.x, 0.0f, impulse.z }), TipOver));
}

CharacterBody::GroundProbe CharacterBody::ProbeGround(JPH::RVec3Arg centerOfMass) const {
    // A small sphere instead of a ray, so the leg doesn't slip through gaps at edges.
    const float probeRadius = radius * 0.5f;
    const float castLength = centerHeight + LegReach - probeRadius;

    const JPH::RShapeCast cast(probeShape, JPH::Vec3::sOne(), JPH::RMat44::sTranslation(centerOfMass),
                               JPH::Vec3(0.0f, -castLength, 0.0f));
    JPH::ClosestHitCollisionCollector<JPH::CastShapeCollector> collector;
    const JPH::IgnoreSingleBodyFilter ignoreSelf(GetBodyID());
    PhysicsWorld::Instance().GetSystem().GetNarrowPhaseQuery().CastShape(
        cast, JPH::ShapeCastSettings(), centerOfMass, collector, {}, {}, ignoreSelf);

    GroundProbe probe;
    if (!collector.HadHit()) {
        return probe;
    }

    probe.hit = true;
    probe.distance = collector.mHit.mFraction * castLength + probeRadius;
    probe.body = collector.mHit.mBodyID2;
    probe.point = centerOfMass + collector.mHit.mContactPointOn2;
    probe.velocity = Bodies().GetPointVelocity(probe.body, probe.point);
    return probe;
}

void CharacterBody::PrePhysicsStep(float deltaTime) {
    PhysicsBody::PrePhysicsStep(deltaTime); // transform edits / kinematic replica
    if (GetMotionType() != JPH::EMotionType::Dynamic) {
        return;
    }

    overrideTimer = std::max(0.0f, overrideTimer - deltaTime);
    jumpTimer = std::max(0.0f, jumpTimer - deltaTime);
    const CharacterTuning& tuning = ActiveTuning();

    JPH::BodyInterface& bodies = Bodies();
    const JPH::BodyID id = GetBodyID();
    const JPH::Vec3 up = JPH::Vec3::sAxisY();
    const float gravity = -PhysicsWorld::Instance().GetSystem().GetGravity().GetY();

    JPH::Vec3 velocity = bodies.GetLinearVelocity(id);
    JPH::Vec3 angularVelocity = bodies.GetAngularVelocity(id);
    const JPH::Quat rotation = bodies.GetRotation(id);

    const GroundProbe ground = ProbeGround(bodies.GetCenterOfMassPosition(id));
    grounded = ground.hit && jumpTimer <= 0.0f;

    // 1. Leg: hold the center of mass at standing height, carrying gravity while grounded.
    if (grounded && tuning.leg.frequency > 0.0f) {
        const float offset = centerHeight - ground.distance;
        const float relativeVelocity = (velocity - ground.velocity).Dot(up);
        const float acceleration = std::clamp(
            SpringAcceleration(tuning.leg, offset, relativeVelocity, deltaTime) + gravity,
            -MaxLegPull, gravity + MaxLegPush);

        const JPH::Vec3 deltaVelocity = up * (acceleration * deltaTime);
        velocity += deltaVelocity;

        // Newton's third law: our weight presses on what we stand on.
        if (bodies.GetMotionType(ground.body) == JPH::EMotionType::Dynamic) {
            bodies.AddImpulse(ground.body, -deltaVelocity * mass, ground.point);
        }
    }

    // 2. Movement. The capsule floats, so there is no ground friction: standing still on a
    //    moving vehicle is the control matching the ground's velocity.
    const JPH::Vec3 input = ToJolt(moveInput);
    if (tuning.control) {
        const JPH::Vec3 horizontal(velocity.GetX(), 0.0f, velocity.GetZ());
        if (grounded) {
            const JPH::Vec3 target = input * moveSpeed + JPH::Vec3(ground.velocity.GetX(), 0.0f, ground.velocity.GetZ());
            const JPH::Vec3 next = MoveTowards(horizontal, target, groundAcceleration * deltaTime);
            velocity = JPH::Vec3(next.GetX(), velocity.GetY(), next.GetZ());

            if (jumpRequested) {
                velocity.SetY(ground.velocity.GetY() + jumpSpeed);
                jumpTimer = JumpLegOffSeconds;
                grounded = false;
            }
        } else if (input.LengthSq() > 0.0f) {
            const JPH::Vec3 next = MoveTowards(horizontal, input * moveSpeed, airAcceleration * deltaTime);
            velocity = JPH::Vec3(next.GetX(), velocity.GetY(), next.GetZ());
        }

        if (input.LengthSq() > 1e-4f) {
            facingYaw = std::atan2(-input.GetX(), -input.GetZ()); // forward is -Z
        }
    }
    jumpRequested = false;

    // 3. Upright: one spring on all three axes towards "straight up, facing facingYaw".
    if (tuning.upright.frequency > 0.0f) {
        JPH::Quat error = JPH::Quat::sRotation(up, facingYaw) * rotation.Conjugated();
        if (error.GetW() < 0.0f) {
            error = -error; // shortest way round
        }

        JPH::Vec3 axis;
        float angle = 0.0f;
        error.GetAxisAngle(axis, angle);

        JPH::Vec3 angularAcceleration = SpringAcceleration(tuning.upright, axis * angle, angularVelocity, deltaTime);
        const float length = angularAcceleration.Length();
        if (length > MaxAngularAcceleration) {
            angularAcceleration *= MaxAngularAcceleration / length;
        }
        angularVelocity += angularAcceleration * deltaTime;
    }

    bodies.SetLinearAndAngularVelocity(id, velocity, angularVelocity);
}

}
