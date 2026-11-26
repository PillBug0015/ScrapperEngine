#include "PoseDrive.h"
#include "Scrapper/Jolt/JoltConversion.h"

#include <Jolt/Physics/Constraints/SwingTwistConstraint.h>

#include <cstdio>


namespace Scrapper {
std::optional<std::vector<PoseLayer>> ResolveLayers(const RagdollAsset& asset, const std::vector<PoseLayerDesc>& layers) {
    std::vector<PoseLayer> resolved;
    for (const PoseLayerDesc& layer : layers) {
        const std::optional<ClipId> clip = asset.FindClip(layer.clip);
        const std::optional<MaskId> mask = asset.FindMask(layer.mask);
        if (!clip || !mask) {
            std::fprintf(stderr, "[Jolt] pose layer %s over %s: no such clip or mask\n", layer.clip.c_str(), layer.mask.c_str());
            return std::nullopt;
        }
        resolved.push_back({ *clip, *mask, layer.driver, layer.fadeSeconds });
    }
    return resolved;
}

namespace {
float Ease(float weight, float target, float fadeSeconds, float deltaTime) {
    const float step = fadeSeconds > 0.0f ? deltaTime / fadeSeconds : 1.0f;
    return weight + Clamp(target - weight, -step, step);
}

// The rotation from the parent that points the aim's axis at its point. Parts rest unrotated,
// so the parent's frame is the rest frame.
Quaternion Aimed(JPH::BodyInterface& bodies, const RagdollAsset& asset, const RagdollInstance& instance, const PoseAim& aim) {
    const PartId parent = *asset.At(aim.part).parent;
    const JPH::Quat parentRotation = bodies.GetRotation(instance.ragdoll->GetBodyID(parent.value));
    const JPH::Vec3 toPoint(ToJoltR(aim.point) - bodies.GetPosition(instance.ragdoll->GetBodyID(aim.part.value)));
    const JPH::Vec3 direction = parentRotation.Conjugated() * toPoint.NormalizedOr(ToJolt(aim.axis));
    return ToRaylib(JPH::Quat::sFromTo(ToJolt(aim.axis), direction));
}

Quaternion Blend(const RagdollAsset& asset, const RagdollPose& pose, PartId part) {
    Quaternion rotation = QuaternionIdentity();
    for (const PoseLayer& layer : pose.layers) {
        const float weight = layer.weight * asset.At(layer.mask).weightByPart[part.value];
        if (weight > 0.0f) rotation = QuaternionSlerp(rotation, asset.At(layer.clip).Sample(part, layer.time), weight);
    }
    return rotation;
}

void Drive(JPH::SwingTwistConstraint& joint, Quaternion target, float torque) {
    joint.GetSwingMotorSettings().SetTorqueLimit(torque);
    joint.GetTwistMotorSettings().SetTorqueLimit(torque);
    joint.SetSwingMotorState(JPH::EMotorState::Position);
    joint.SetTwistMotorState(JPH::EMotorState::Position);
    joint.SetTargetOrientationBS(ToJolt(target));
}
}

void DrivePose(JPH::BodyInterface& bodies, const RagdollAsset& asset, RagdollInstance& instance, RagdollPose& pose, float deltaTime) {
    for (PoseLayer& layer : pose.layers) {
        layer.weight = Ease(layer.weight, layer.target, layer.fadeSeconds, deltaTime);
        layer.time += deltaTime * layer.pace;
    }
    for (PoseAim& aim : pose.aims) aim.weight = Ease(aim.weight, aim.target, aim.fadeSeconds, deltaTime);

    for (std::size_t i = 0; i < asset.parts.size(); ++i) {
        const int index = asset.settings->GetConstraintIndexForBodyIndex(static_cast<int>(i));
        JPH::TwoBodyConstraint* constraint = index >= 0 ? instance.ragdoll->GetConstraint(index) : nullptr;
        if (constraint == nullptr || constraint->GetSubType() != JPH::EConstraintSubType::SwingTwist) continue;

        const PartId part{ static_cast<std::uint16_t>(i) };
        Quaternion target = Blend(asset, pose, part);
        const PoseAim* aim = pose.Aim(part);
        if (aim != nullptr && aim->weight > 0.0f) target = QuaternionSlerp(target, Aimed(bodies, asset, instance, *aim), aim->weight);
        Drive(static_cast<JPH::SwingTwistConstraint&>(*constraint), target, asset.motorTorque * pose.strength);
    }
}

}
