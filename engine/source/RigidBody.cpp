#include "Physics/RigidBody.h"
#include "Physics/JoltConversion.h"
#include "Physics/PhysicsWorld.h"

#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyInterface.h>


namespace ScrapperEngine {
RigidBody::RigidBody(const JPH::BodyCreationSettings& settings)
    : PhysicsBody(settings.mMotionType), settings(settings) {}

RigidBody::RigidBody(JPH::RefConst<JPH::Shape> shape, JPH::EMotionType motion)
    : PhysicsBody(motion),
      settings(shape, JPH::RVec3::sZero(), JPH::Quat::sIdentity(), motion,
               motion == JPH::EMotionType::Static ? PhysicsLayers::Static : PhysicsLayers::Moving) {}

JPH::BodyID RigidBody::CreateBody(Vector3 position, Quaternion rotation) {
    settings.mPosition = ToJoltR(position);
    settings.mRotation = ToJolt(rotation);
    settings.mMotionType = GetMotionType();

    const JPH::Body* body = PhysicsWorld::Instance().GetBodyInterface().CreateBody(settings);
    return body != nullptr ? body->GetID() : JPH::BodyID();
}

}
