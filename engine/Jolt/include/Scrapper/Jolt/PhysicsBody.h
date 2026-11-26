#pragma once

#include "Scrapper/Assets/Model.h"
#include "Scrapper/Core/Math.h"
#include "Scrapper/Core/Transform.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Body/MotionType.h>

#include <vector>


namespace Scrapper {
// Authored by game code: what body to create. Pose comes from the entity's transform;
// changes after the body exists are ignored.
struct RigidBody {
    JPH::BodyCreationSettings settings;

    // Common case: object layer follows the motion type.
    static RigidBody Make(JPH::RefConst<JPH::Shape> shape, JPH::EMotionType motion);
    // One compound of convex hulls, e.g. a model's COL_ pieces. mass 0 keeps the shapes' density.
    static RigidBody FromConvexParts(const std::vector<ConvexPart>& parts, JPH::EMotionType motion, float mass = 0.0f);
};

// The last local pose exchanged with Jolt: a LocalTransform that differs was moved by game code.
struct SyncedPose {
    Vector3 position = { 0.0f, 0.0f, 0.0f };
    Quaternion rotation = { 0.0f, 0.0f, 0.0f, 1.0f };

    void Remember(const LocalTransform& local) {
        position = local.position;
        rotation = local.rotation;
    }
    // Exact on purpose: the synced pose is what the module itself last wrote.
    bool Matches(const LocalTransform& local) const {
        return local.position.x == position.x && local.position.y == position.y && local.position.z == position.z &&
               local.rotation.x == rotation.x && local.rotation.y == rotation.y && local.rotation.z == rotation.z &&
               local.rotation.w == rotation.w;
    }
};

// Written by the Jolt module, read by everyone else.
// The LocalTransform stays the source of truth for game code: moving a dynamic or static body's
// LocalTransform yourself teleports it; a kinematic body follows its LocalTransform with velocity.
struct PhysicsBody {
    JPH::BodyID id;
    JPH::EMotionType authoredMotion = JPH::EMotionType::Dynamic;
    JPH::EMotionType motion = JPH::EMotionType::Dynamic; // authored, or kinematic under RemoteAuthority
    bool inWorld = false;
    bool ragdollPart = false; // its ragdoll adds, removes, moves and destroys it as a whole
    Vector3 linearVelocity = { 0.0f, 0.0f, 0.0f };
    Vector3 angularVelocity = { 0.0f, 0.0f, 0.0f };
    SyncedPose synced;
};

// Game code's requests for the next physics step. Applied to dynamic bodies, then cleared.
// On a Ragdoll entity they act on the ragdoll as a whole.
struct PhysicsCommands {
    Vector3 impulse = { 0.0f, 0.0f, 0.0f };
    Vector3 angularImpulse = { 0.0f, 0.0f, 0.0f };
    Vector3 force = { 0.0f, 0.0f, 0.0f };  // held through every step of this frame
    Vector3 torque = { 0.0f, 0.0f, 0.0f }; // held through every step of this frame
    bool overrideLinearVelocity = false;
    Vector3 linearVelocity = { 0.0f, 0.0f, 0.0f };
};

}
