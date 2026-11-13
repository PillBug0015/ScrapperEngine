#pragma once

#include "Physics/PhysicsBody.h"

#include <Jolt/Physics/Body/BodyCreationSettings.h>


namespace ScrapperEngine {
// General rigid body built from Jolt BodyCreationSettings. Pose comes from the transform.
class RigidBody : public PhysicsBody {
public:
    // Full control: any Jolt option (allowed DOFs, friction, mass override...).
    explicit RigidBody(const JPH::BodyCreationSettings& settings);
    // Common case: object layer is chosen from the motion type.
    RigidBody(JPH::RefConst<JPH::Shape> shape, JPH::EMotionType motion);

protected:
    JPH::BodyID CreateBody(Vector3 position, Quaternion rotation) override;

private:
    JPH::BodyCreationSettings settings;
};

}
