#include "Scrapper/Jolt/PhysicsBody.h"
#include "Scrapper/Jolt/JoltConversion.h"
#include "Scrapper/Jolt/JoltWorld.h"

#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>

#include <cstdio>


namespace Scrapper {
RigidBody RigidBody::Make(JPH::RefConst<JPH::Shape> shape, JPH::EMotionType motion) {
    return { JPH::BodyCreationSettings(shape, JPH::RVec3::sZero(), JPH::Quat::sIdentity(), motion,
                                       motion == JPH::EMotionType::Static ? PhysicsLayers::Static : PhysicsLayers::Moving) };
}

RigidBody RigidBody::FromConvexParts(const std::vector<ConvexPart>& parts, JPH::EMotionType motion, float mass) {
    JPH::StaticCompoundShapeSettings compound;
    for (const ConvexPart& part : parts) {
        JPH::Array<JPH::Vec3> points;
        for (const Vector3& p : part.positions) points.push_back(ToJolt(p));
        compound.AddShape(JPH::Vec3::sZero(), JPH::Quat::sIdentity(), new JPH::ConvexHullShapeSettings(points));
    }

    JPH::ShapeSettings::ShapeResult result = compound.Create();
    if (result.HasError()) {
        std::fprintf(stderr, "[Jolt] collider pieces rejected (%s); using a placeholder box\n", result.GetError().c_str());
        return Make(new JPH::BoxShape(JPH::Vec3::sReplicate(0.5f)), motion);
    }

    RigidBody body = Make(result.Get(), motion);
    if (mass > 0.0f) {
        body.settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
        body.settings.mMassPropertiesOverride.mMass = mass;
    }
    return body;
}

}
