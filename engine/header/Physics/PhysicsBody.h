#pragma once

#include "Core/Component/Component.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Body/MotionType.h>
#include <raylib.h>


namespace ScrapperEngine {
class Transform3D;

// A Jolt body bound to the sibling Transform3D. The transform stays the single source of truth for game code:
//  Dynamic   : physics drives the transform; moving the transform yourself teleports the body
//  Kinematic : the transform drives the body (moved with velocity, so it pushes others)
//  Static    : moving the transform teleports the body
// Subclasses only decide how the body is created and what happens around each physics step.
class PhysicsBody : public Component {
public:
    JPH::BodyID GetBodyID() const { return bodyID; }
    JPH::EMotionType GetMotionType() const { return motionType; }
    void SetMotionType(JPH::EMotionType motion);

    void AddForce(Vector3 force);
    void AddImpulse(Vector3 impulse);
    void AddTorque(Vector3 torque);
    void AddAngularImpulse(Vector3 impulse);
    Vector3 GetLinearVelocity() const;
    void SetLinearVelocity(Vector3 velocity);

    void Start() override;
    void OnEnable() override;
    void OnDisable() override;
    void OnDestroy() override;

protected:
    explicit PhysicsBody(JPH::EMotionType motion) : motionType(motion) {}

    // Create the body at this pose without adding it to the world.
    virtual JPH::BodyID CreateBody(Vector3 position, Quaternion rotation) = 0;
    virtual void DestroyBody();

    // Around every fixed step. The base pre-step pushes transform edits into the body.
    virtual void PrePhysicsStep(float deltaTime);
    virtual void PostPhysicsStep() {}

    Transform3D* GetTransform() const { return transform; }

private:
    friend class PhysicsWorld;
    void PullDynamicPose(); // once per frame, after all steps
    void RememberSyncedPose();

    JPH::EMotionType motionType;
    JPH::BodyID bodyID;
    Transform3D* transform = nullptr;
    bool inWorld = false;

    // Last pose exchanged with Jolt. A transform that no longer matches it was moved by game code.
    Vector3 syncedPosition = { 0.0f, 0.0f, 0.0f };
    Quaternion syncedRotation = { 0.0f, 0.0f, 0.0f, 1.0f };
};

}
