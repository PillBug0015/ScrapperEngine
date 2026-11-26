#include "Scrapper/Jolt/Ragdoll.h"
#include "Scrapper/Jolt/JoltConversion.h"
#include "Scrapper/Jolt/JoltWorld.h"
#include "ShapeModel.h"

#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Constraints/SwingTwistConstraint.h>
#include <Jolt/Skeleton/Skeleton.h>

#include <algorithm>
#include <cfloat>
#include <cstdio>


namespace Scrapper {
namespace {
// Along the part, from its joint; a part with no length is a ball and hangs straight down.
JPH::Vec3 Axis(const RagdollPartDesc& part) {
    return (ToJolt(part.to) - ToJolt(part.from)).NormalizedOr(-JPH::Vec3::sAxisY());
}

JPH::RefConst<JPH::Shape> MakeShape(const RagdollPartDesc& part) {
    const JPH::Vec3 span = ToJolt(part.to) - ToJolt(part.from);
    const float halfLength = 0.5f * span.Length();
    const JPH::Shape* core = halfLength > 1e-3f ? static_cast<JPH::Shape*>(new JPH::CapsuleShape(halfLength, part.radius))
                                                : new JPH::SphereShape(part.radius);
    return new JPH::RotatedTranslatedShape(0.5f * span, JPH::Quat::sFromTo(JPH::Vec3::sAxisY(), Axis(part)), core);
}

JPH::Ref<JPH::SwingTwistConstraintSettings> MakeJoint(const RagdollPartDesc& part, const RagdollDesc& desc) {
    JPH::Ref<JPH::SwingTwistConstraintSettings> joint = new JPH::SwingTwistConstraintSettings;
    joint->mPosition1 = joint->mPosition2 = ToJoltR(part.from);
    joint->mTwistAxis1 = joint->mTwistAxis2 = Axis(part);
    joint->mPlaneAxis1 = joint->mPlaneAxis2 = Axis(part).GetNormalizedPerpendicular();
    joint->mNormalHalfConeAngle = joint->mPlaneHalfConeAngle = JPH::DegreesToRadians(part.swingDegrees);
    joint->mTwistMinAngle = -JPH::DegreesToRadians(part.twistDegrees);
    joint->mTwistMaxAngle = JPH::DegreesToRadians(part.twistDegrees);
    joint->mSwingMotorSettings = joint->mTwistMotorSettings = JPH::MotorSettings(desc.motorFrequency, 1.0f, FLT_MAX, desc.motorTorque);
    return joint;
}

RagdollAsset::Clip Resolve(const RagdollClip& clip, const RagdollAsset& asset) {
    RagdollAsset::Clip resolved{ clip.name, clip.duration, clip.loop, std::vector<std::vector<RagdollClip::Key>>(asset.parts.size()) };
    for (const RagdollClip::Track& track : clip.tracks) {
        const int part = asset.FindPart(track.part);
        if (part >= 0) resolved.keysByPart[part] = track.keys;
        else std::fprintf(stderr, "[Jolt] clip %s: no part named %s\n", clip.name.c_str(), track.part.c_str());
    }
    return resolved;
}
}

int RagdollAsset::FindPart(const std::string& name) const {
    for (std::size_t i = 0; i < parts.size(); ++i) {
        if (parts[i].name == name) return static_cast<int>(i);
    }
    return -1;
}

const RagdollAsset::Clip* RagdollAsset::FindClip(const std::string& name) const {
    for (const Clip& clip : clips) {
        if (clip.name == name) return &clip;
    }
    return nullptr;
}

std::shared_ptr<const RagdollAsset> MakeRagdollAsset(const RagdollDesc& desc, const std::vector<RagdollClip>& clips) {
    auto asset = std::make_shared<RagdollAsset>();
    asset->settings = new JPH::RagdollSettings;
    asset->settings->mSkeleton = new JPH::Skeleton;
    asset->motorTorque = desc.motorTorque;

    std::vector<JPH::Mat44> rest;
    for (const RagdollPartDesc& part : desc.parts) {
        const int parent = part.parent.empty() ? -1 : asset->FindPart(part.parent);
        if ((parent < 0) != part.parent.empty() || (parent < 0) != asset->parts.empty()) {
            std::fprintf(stderr, "[Jolt] ragdoll part %s: one root first, then parents before their children\n", part.name.c_str());
            return nullptr;
        }

        asset->settings->mSkeleton->AddJoint(part.name, parent);
        JPH::RagdollSettings::Part& body = asset->settings->mParts.emplace_back();
        body.SetShape(MakeShape(part));
        body.mPosition = ToJoltR(part.from);
        body.mMotionType = JPH::EMotionType::Dynamic;
        body.mObjectLayer = PhysicsLayers::Moving;
        body.mAllowSleeping = false; // characters push their parts every step
        body.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
        body.mMassPropertiesOverride.mMass = part.mass;
        if (parent >= 0) body.mToParent = MakeJoint(part, desc);

        asset->parts.push_back({ part.name, parent, part.from, QuaternionIdentity(), MakeShapeModel(*body.GetShape(), part.name) });
        rest.push_back(JPH::Mat44::sTranslation(ToJolt(part.from)));
    }

    // Parts touching at rest would fight their own joints: they never collide with each other.
    JPH::RagdollSettings& settings = *asset->settings;
    settings.DisableParentChildCollisions(rest.data(), 0.02f);
    settings.Stabilize();
    settings.CalculateConstraintPriorities();
    settings.CalculateBodyIndexToConstraintIndex();
    settings.CalculateConstraintIndexToBodyIdxPair();

    float lowest = FLT_MAX;
    for (std::size_t i = 0; i < settings.mParts.size(); ++i) {
        const JPH::Shape& shape = *settings.mParts[i].GetShape();
        lowest = std::min(lowest, shape.GetWorldSpaceBounds(rest[i].PreTranslated(shape.GetCenterOfMass()), JPH::Vec3::sOne()).mMin.GetY());
    }
    for (RagdollAsset::Part& part : asset->parts) part.position.y -= lowest;

    for (const RagdollClip& clip : clips) asset->clips.push_back(Resolve(clip, *asset));
    return asset;
}

}
