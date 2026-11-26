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

std::optional<RagdollAsset::Clip> Resolve(const RagdollClip& clip, const RagdollAsset& asset) {
    RagdollAsset::Clip resolved{ clip.name, clip.duration, clip.loop, clip.stride, std::vector<std::vector<RagdollClip::Key>>(asset.parts.size()) };
    for (const RagdollClip::Track& track : clip.tracks) {
        const std::optional<PartId> part = asset.FindPart(track.part);
        if (!part) {
            std::fprintf(stderr, "[Jolt] clip %s: no part named %s\n", clip.name.c_str(), track.part.c_str());
            return std::nullopt;
        }
        resolved.keysByPart[part->value] = track.keys;
    }
    return resolved;
}

// Parents come before their children, so one pass carries a root's membership down to its subtree.
std::optional<RagdollAsset::Mask> Resolve(const RagdollMaskDesc& mask, const RagdollAsset& asset) {
    RagdollAsset::Mask resolved{ mask.name, std::vector<float>(asset.parts.size(), 0.0f) };
    for (const std::string& name : mask.roots) {
        const std::optional<PartId> root = asset.FindPart(name);
        if (!root) {
            std::fprintf(stderr, "[Jolt] mask %s: no part named %s\n", mask.name.c_str(), name.c_str());
            return std::nullopt;
        }
        resolved.weightByPart[root->value] = 1.0f;
    }
    for (std::size_t i = 0; i < asset.parts.size(); ++i) {
        const std::optional<PartId> parent = asset.parts[i].parent;
        if (parent && resolved.weightByPart[parent->value] > 0.0f) resolved.weightByPart[i] = 1.0f;
    }
    return resolved;
}

template <typename Id, typename Item>
std::optional<Id> FindByName(const std::vector<Item>& items, std::string_view name) {
    for (std::size_t i = 0; i < items.size(); ++i) {
        if (items[i].name == name) return Id{ static_cast<std::uint16_t>(i) };
    }
    return std::nullopt;
}
}

std::optional<PartId> RagdollAsset::FindPart(std::string_view name) const { return FindByName<PartId>(parts, name); }
std::optional<ClipId> RagdollAsset::FindClip(std::string_view name) const { return FindByName<ClipId>(clips, name); }
std::optional<MaskId> RagdollAsset::FindMask(std::string_view name) const { return FindByName<MaskId>(masks, name); }

std::shared_ptr<const RagdollAsset> MakeRagdollAsset(const RagdollDesc& desc, const std::vector<RagdollClip>& clips) {
    auto asset = std::make_shared<RagdollAsset>();
    asset->settings = new JPH::RagdollSettings;
    asset->settings->mSkeleton = new JPH::Skeleton;
    asset->motorTorque = desc.motorTorque;

    std::vector<JPH::Mat44> rest;
    for (const RagdollPartDesc& part : desc.parts) {
        const std::optional<PartId> parent = part.parent.empty() ? std::nullopt : asset->FindPart(part.parent);
        if (parent.has_value() == part.parent.empty() || parent.has_value() == asset->parts.empty()) {
            std::fprintf(stderr, "[Jolt] ragdoll part %s: one root first, then parents before their children\n", part.name.c_str());
            return nullptr;
        }

        asset->settings->mSkeleton->AddJoint(part.name, parent ? parent->value : -1);
        JPH::RagdollSettings::Part& body = asset->settings->mParts.emplace_back();
        body.SetShape(MakeShape(part));
        body.mPosition = ToJoltR(part.from);
        body.mMotionType = JPH::EMotionType::Dynamic;
        body.mObjectLayer = PhysicsLayers::Moving;
        body.mAllowSleeping = false; // characters push their parts every step
        body.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
        body.mMassPropertiesOverride.mMass = part.mass;
        if (parent) body.mToParent = MakeJoint(part, desc);

        asset->parts.push_back({ part.name, parent, part.from, QuaternionIdentity(), ToRaylib(Axis(part)), MakeShapeModel(*body.GetShape(), part.name) });
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

    for (const RagdollClip& clip : clips) {
        std::optional<RagdollAsset::Clip> resolved = Resolve(clip, *asset);
        if (!resolved) return nullptr;
        asset->clips.push_back(std::move(*resolved));
    }
    for (const RagdollMaskDesc& mask : desc.masks) {
        std::optional<RagdollAsset::Mask> resolved = Resolve(mask, *asset);
        if (!resolved) return nullptr;
        asset->masks.push_back(std::move(*resolved));
    }
    return asset;
}

}
