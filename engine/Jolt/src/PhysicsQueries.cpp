#include "Scrapper/Jolt/PhysicsQueries.h"
#include "BodyEntity.h"
#include "BodyFilters.h"
#include "RagdollBody.h"
#include "Scrapper/Core/World.h"
#include "Scrapper/Jolt/JoltConversion.h"

#include <Jolt/Physics/Collision/CollideShape.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/NarrowPhaseQuery.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/PhysicsSystem.h>


namespace Scrapper {
std::optional<Touch> FindTouch(World& world, Vector3 center, float radius, entt::entity ignore) {
    JoltWorld& jolt = world.Resource<JoltWorld>();
    const entt::registry& registry = world.Registry();

    JPH::Array<JPH::BodyID> ignored;
    if (const RagdollInstance* ragdoll = registry.try_get<RagdollInstance>(ignore); ragdoll != nullptr && ragdoll->ragdoll != nullptr) {
        ignored = ragdoll->ragdoll->GetBodyIDs();
    } else if (const PhysicsBody* body = registry.try_get<PhysicsBody>(ignore)) {
        ignored.push_back(body->id);
    }

    JPH::SphereShape sphere(radius);
    sphere.SetEmbedded();
    const JPH::RVec3 base = ToJoltR(center);
    JPH::ClosestHitCollisionCollector<JPH::CollideShapeCollector> collector;
    jolt.System().GetNarrowPhaseQuery().CollideShape(&sphere, JPH::Vec3::sOne(), JPH::RMat44::sTranslation(base), JPH::CollideShapeSettings(),
                                                     base, collector, {}, {}, IgnoreBodies(ignored));
    if (!collector.HadHit()) return std::nullopt;

    const JPH::Vec3 point = collector.mHit.mContactPointOn2;
    return Touch{ ToEntity(jolt.Bodies().GetUserData(collector.mHit.mBodyID2)), ToRaylib(JPH::Vec3(base) + point) };
}

}
