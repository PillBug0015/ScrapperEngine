#pragma once

#include "Scrapper/Core/Math.h"
#include "Scrapper/Jolt/PhysicsBody.h"
#include "Scrapper/Jolt/Ragdoll.h"

#include <vector>


namespace Scrapper {
// Spring as frequency (Hz) and damping ratio, applied as acceleration so it ignores mass.
// dampingRatio 1 settles without overshoot, below 1 wobbles.
struct SpringParams {
    float frequency = 0.0f;
    float dampingRatio = 1.0f;
};

// Stability drops to `factor`, then recovers smoothly to 1 over `duration` seconds.
struct Stun {
    float factor = 0.0f;
    float duration = 0.0f;
    float elapsed = 0.0f;
};

// World resource: scales every character's springs at once.
struct CharacterSpringScale {
    float stiffness = 1.0f;
    float damping = 1.0f;
};

// Physics-driven character on the entity's Ragdoll ("floating ragdoll"): springs hold its core part
// up and upright, and it drives the Moving/Airborne/Always layers of the entity's RagdollPose.
// Stability (0..1) scales all of it together: 1 stands firm, lower wobbles, 0 is limp.
// The leg pushes back on what it stands on, and it won't walk away from what its parts Grip.
struct SpringCharacter {
    PartId core = RagdollAsset::Root; // the part the springs hold

    float moveSpeed = 3.0f;
    float jumpSpeed = 5.0f;
    float groundAcceleration = 30.0f; // m/s^2 towards the target velocity while standing
    float airAcceleration = 6.0f;     // steering in the air, only while there is input
    float hover = 0.03f;              // feet kept this far off the ground, so they don't scrape
    SpringParams upright = { 3.0f, 0.9f }; // all three rotation axes: stand straight, face facingYaw
    SpringParams leg = { 5.0f, 0.9f };     // ride height above the ground

    // Intent, written by game code.
    Vector3 moveInput = { 0.0f, 0.0f, 0.0f }; // world-space horizontal direction, length <= 1
    float facingYaw = 0.0f;                   // radians around +Y; 0 faces -Z
    bool jumpRequested = false;               // consumed by the next physics step
    std::vector<Stun> stuns;                  // see Characters::AddStun

    // State, written by the Jolt module.
    float stability = 1.0f; // the lowest active stun
    bool grounded = false;
    float jumpTimer = 0.0f;
};

namespace Characters {
// Overlapping stuns don't add up: the lowest one wins.
void AddStun(SpringCharacter& character, float seconds, float factor);
// Shoved and knocked limp; also tips over in the push direction so it tumbles. The commands go on the
// character's entity, where they push the whole ragdoll.
void Knockback(SpringCharacter& character, PhysicsCommands& commands, Vector3 impulse, float stunSeconds);
}

}
