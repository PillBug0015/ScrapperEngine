#include "Physics/PhysicsBody.h"
#include "Physics/JoltConversion.h"
#include "Physics/PhysicsWorld.h"
#include "Core/GameObject.h"
#include "Core/TransformSystem/Transform3D.h"

#include <Jolt/Physics/Body/BodyInterface.h>


namespace ScrapperEngine {
namespace {
JPH::BodyInterface& Bodies() {
    return PhysicsWorld::Instance().GetBodyInterface();
}
}

void PhysicsBody::Start() {
    transform = owner->GetComponent<Transform3D>();
    if (transform == nullptr) {
        TraceLog(LOG_WARNING, "PhysicsBody: owner has no Transform3D, body not created");
        return;
    }

    bodyID = CreateBody(transform->GetWorldPosition(), transform->GetWorldRotation());
    if (bodyID.IsInvalid()) {
        TraceLog(LOG_WARNING, "PhysicsBody: Jolt body limit reached");
        return;
    }
    RememberSyncedPose();
}

void PhysicsBody::OnEnable() {
    if (bodyID.IsInvalid() || inWorld) {
        return;
    }

    // The transform may have moved while the body was out of the world.
    Bodies().SetPositionAndRotation(bodyID, ToJoltR(transform->GetWorldPosition()),
                                    ToJolt(transform->GetWorldRotation()), JPH::EActivation::DontActivate);
    Bodies().AddBody(bodyID, motionType == JPH::EMotionType::Static
        ? JPH::EActivation::DontActivate
        : JPH::EActivation::Activate);
    RememberSyncedPose();

    PhysicsWorld::Instance().Register(this);
    inWorld = true;
}

void PhysicsBody::OnDisable() {
    if (!inWorld) {
        return;
    }

    PhysicsWorld::Instance().Unregister(this);
    Bodies().RemoveBody(bodyID);
    inWorld = false;
}

void PhysicsBody::OnDestroy() {
    OnDisable();

    if (!bodyID.IsInvalid()) {
        DestroyBody();
        bodyID = JPH::BodyID();
    }
}

void PhysicsBody::DestroyBody() {
    Bodies().DestroyBody(bodyID);
}

void PhysicsBody::SetMotionType(JPH::EMotionType motion) {
    motionType = motion;
    if (!bodyID.IsInvalid()) {
        Bodies().SetMotionType(bodyID, motion, JPH::EActivation::Activate);
    }
}

void PhysicsBody::AddForce(Vector3 force) {
    Bodies().AddForce(bodyID, ToJolt(force));
}

void PhysicsBody::AddImpulse(Vector3 impulse) {
    Bodies().AddImpulse(bodyID, ToJolt(impulse));
}

void PhysicsBody::AddTorque(Vector3 torque) {
    Bodies().AddTorque(bodyID, ToJolt(torque));
}

void PhysicsBody::AddAngularImpulse(Vector3 impulse) {
    Bodies().AddAngularImpulse(bodyID, ToJolt(impulse));
}

Vector3 PhysicsBody::GetLinearVelocity() const {
    return ToRaylib(Bodies().GetLinearVelocity(bodyID));
}

void PhysicsBody::SetLinearVelocity(Vector3 velocity) {
    Bodies().SetLinearVelocity(bodyID, ToJolt(velocity));
}

void PhysicsBody::RememberSyncedPose() {
    syncedPosition = transform->GetWorldPosition();
    syncedRotation = transform->GetWorldRotation();
}

void PhysicsBody::PrePhysicsStep(float deltaTime) {
    const Vector3 position = transform->GetWorldPosition();
    const Quaternion rotation = transform->GetWorldRotation();

    if (motionType == JPH::EMotionType::Kinematic) {
        Bodies().MoveKinematic(bodyID, ToJoltR(position), ToJolt(rotation), deltaTime);
        return;
    }

    // Exact comparison on purpose: the synced pose is read back from this very transform.
    const bool moved =
        position.x != syncedPosition.x || position.y != syncedPosition.y || position.z != syncedPosition.z ||
        rotation.x != syncedRotation.x || rotation.y != syncedRotation.y ||
        rotation.z != syncedRotation.z || rotation.w != syncedRotation.w;

    if (moved) {
        Bodies().SetPositionAndRotation(bodyID, ToJoltR(position), ToJolt(rotation), JPH::EActivation::Activate);
        syncedPosition = position;
        syncedRotation = rotation;
    }
}

void PhysicsBody::PullDynamicPose() {
    if (motionType != JPH::EMotionType::Dynamic || !Bodies().IsActive(bodyID)) {
        return; // sleeping bodies haven't moved
    }

    JPH::RVec3 position;
    JPH::Quat rotation;
    Bodies().GetPositionAndRotation(bodyID, position, rotation);
    transform->SetWorldPose(ToRaylib(position), ToRaylib(rotation));
    RememberSyncedPose();
}

}
