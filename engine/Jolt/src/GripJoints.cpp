#include "GripJoints.h"
#include "Scrapper/Jolt/Grip.h"
#include "Scrapper/Jolt/JoltConversion.h"
#include "Scrapper/Jolt/PhysicsBody.h"

#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Constraints/SixDOFConstraint.h>
#include <Jolt/Physics/PhysicsSystem.h>

#include <cfloat>
#include <vector>


namespace Scrapper {
namespace GripJoints {
namespace {
constexpr float PullFrequency = 10.0f; // Hz: how briskly a grip closes the gap, up to its strength

bool InWorld(const entt::registry& registry, entt::entity entity) {
    const PhysicsBody* body = registry.valid(entity) ? registry.try_get<PhysicsBody>(entity) : nullptr;
    return body != nullptr && body->inWorld;
}

// Translation motors towards "anchors together", force-capped at the grip's strength; rotation free.
JPH::Ref<JPH::Constraint> MakeJoint(JoltWorld& jolt, JPH::BodyID hand, JPH::BodyID target, const Grip& grip) {
    JPH::BodyInterface& bodies = jolt.Bodies();
    JPH::SixDOFConstraintSettings settings;
    settings.mPosition1 = bodies.GetWorldTransform(hand) * ToJolt(grip.anchor);
    settings.mPosition2 = bodies.GetWorldTransform(target) * ToJolt(grip.targetAnchor);
    for (int axis = JPH::SixDOFConstraintSettings::TranslationX; axis <= JPH::SixDOFConstraintSettings::TranslationZ; ++axis) {
        settings.mMotorSettings[axis] = JPH::MotorSettings(PullFrequency, 1.0f, grip.strength, 0.0f);
    }

    auto* joint = static_cast<JPH::SixDOFConstraint*>(bodies.CreateConstraint(&settings, hand, target));
    for (int axis = JPH::SixDOFConstraintSettings::TranslationX; axis <= JPH::SixDOFConstraintSettings::TranslationZ; ++axis) {
        joint->SetMotorState(static_cast<JPH::SixDOFConstraintSettings::EAxis>(axis), JPH::EMotorState::Position);
    }
    jolt.System().AddConstraint(joint);
    for (const JPH::BodyID id : { hand, target }) {
        if (bodies.GetMotionType(id) != JPH::EMotionType::Static) bodies.ActivateBody(id);
    }
    return joint;
}
}

void Sync(entt::registry& registry, JoltWorld& jolt) {
    std::vector<entt::entity> ended;
    for (auto [entity, joint] : registry.view<GripJoint>().each()) {
        const Grip* grip = registry.try_get<Grip>(entity);
        if (grip == nullptr || grip->target != joint.target || !InWorld(registry, entity) || !InWorld(registry, joint.target)) {
            ended.push_back(entity);
        }
    }
    registry.remove<GripJoint>(ended.begin(), ended.end());

    std::vector<entt::entity> started;
    for (auto [entity, grip] : registry.view<Grip>(entt::exclude<GripJoint>).each()) {
        if (InWorld(registry, entity) && InWorld(registry, grip.target)) started.push_back(entity);
    }
    for (entt::entity entity : started) {
        const Grip& grip = registry.get<Grip>(entity);
        const JPH::BodyID hand = registry.get<PhysicsBody>(entity).id;
        const JPH::BodyID target = registry.get<PhysicsBody>(grip.target).id;
        registry.emplace<GripJoint>(entity, MakeJoint(jolt, hand, target, grip), grip.target);
    }
}

std::vector<JPH::Vec3> Pulls(const entt::registry& registry, JoltWorld& jolt, const std::vector<entt::entity>& parts) {
    JPH::BodyInterface& bodies = jolt.Bodies();
    std::vector<JPH::Vec3> pulls;
    for (entt::entity part : parts) {
        const Grip* grip = registry.try_get<Grip>(part);
        if (grip == nullptr || !registry.all_of<GripJoint>(part)) continue;
        const JPH::RVec3 hand = bodies.GetWorldTransform(registry.get<PhysicsBody>(part).id) * ToJolt(grip->anchor);
        const JPH::RVec3 hold = bodies.GetWorldTransform(registry.get<PhysicsBody>(grip->target).id) * ToJolt(grip->targetAnchor);
        pulls.push_back(JPH::Vec3(hold - hand));
    }
    return pulls;
}

void OnDestroyed(entt::registry& registry, entt::entity entity) {
    JoltWorld* jolt = registry.ctx().find<JoltWorld>();
    const GripJoint& joint = registry.get<GripJoint>(entity);
    if (jolt != nullptr && joint.constraint != nullptr) jolt->System().RemoveConstraint(joint.constraint);
}
}

}
