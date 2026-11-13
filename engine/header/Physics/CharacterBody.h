#pragma once

#include "Physics/PhysicsBody.h"

#include <Jolt/Physics/Collision/Shape/Shape.h>


namespace ScrapperEngine {
// Spring as frequency (Hz) and damping ratio, applied as acceleration so it ignores mass.
// frequency 0 turns the spring off; dampingRatio 1 settles without overshoot, below 1 wobbles.
struct SpringParams {
    float frequency = 0.0f;
    float dampingRatio = 1.0f;
};

struct CharacterTuning {
    SpringParams upright; // all three rotation axes: stand straight, face the move direction
    SpringParams leg;     // ride height above the ground
    bool control = true;  // whether move/jump input does anything
};

namespace CharacterTunings {
inline constexpr CharacterTuning Normal = { { 3.0f, 0.9f }, { 5.0f, 0.9f }, true };
inline constexpr CharacterTuning Wobbly = { { 1.2f, 0.2f }, { 4.0f, 0.4f }, true };
inline constexpr CharacterTuning Limp = { { 0.0f, 0.0f }, { 0.0f, 0.0f }, false }; // tumbles freely
}

// Physics-driven character: a free capsule held up by springs ("floating capsule").
// Every comedic state is just a different tuning. The transform origin is at the feet.
// The leg pushes back on whatever it stands on, so crew weight loads the vehicle they ride.
class CharacterBody : public PhysicsBody {
public:
    // Scale every character's springs at once.
    static inline float globalStiffness = 1.0f;
    static inline float globalDamping = 1.0f;

    float moveSpeed = 4.0f;
    float jumpSpeed = 5.0f;
    float groundAcceleration = 40.0f; // m/s^2 towards the target velocity while standing
    float airAcceleration = 8.0f;     // steering in the air, only while there is input

    CharacterBody(float height = 1.8f, float radius = 0.35f, float mass = 80.0f, float legLength = 0.3f);

    // World-space horizontal direction, length <= 1. Kept until changed.
    void SetMoveInput(Vector3 direction) { moveInput = direction; }
    void Jump() { jumpRequested = true; }

    void SetTuning(const CharacterTuning& tuning) { baseTuning = tuning; }
    // Temporary state (wobbling, limp...) that falls back to the base tuning afterwards.
    void OverrideTuning(const CharacterTuning& tuning, float seconds);
    // Goes limp and gets shoved; also tips over in the push direction so it tumbles.
    void Knockback(Vector3 impulse, float limpSeconds);

    bool IsGrounded() const { return grounded; }
    bool HasControl() const { return ActiveTuning().control; }

protected:
    JPH::BodyID CreateBody(Vector3 position, Quaternion rotation) override;
    void PrePhysicsStep(float deltaTime) override;

private:
    struct GroundProbe {
        bool hit = false;
        float distance = 0.0f; // from the center of mass down to the ground
        JPH::BodyID body;
        JPH::RVec3 point;
        JPH::Vec3 velocity = JPH::Vec3::sZero(); // of the ground under the feet
    };

    const CharacterTuning& ActiveTuning() const { return overrideTimer > 0.0f ? overrideTuning : baseTuning; }
    GroundProbe ProbeGround(JPH::RVec3Arg centerOfMass) const;

    float height;
    float radius;
    float mass;
    float legLength;
    float centerHeight = 0.0f; // feet to center of mass when upright
    JPH::RefConst<JPH::Shape> probeShape;

    CharacterTuning baseTuning = CharacterTunings::Normal;
    CharacterTuning overrideTuning = CharacterTunings::Normal;
    float overrideTimer = 0.0f;

    Vector3 moveInput = { 0.0f, 0.0f, 0.0f };
    float facingYaw = 0.0f;
    bool jumpRequested = false;
    float jumpTimer = 0.0f; // leg stays off right after a jump so it doesn't pull the body back down
    bool grounded = false;
};

}
