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
void Advance(PoseLayer& layer, float deltaTime) {
    const float step = layer.fadeSeconds > 0.0f ? deltaTime / layer.fadeSeconds : 1.0f;
    layer.weight += Clamp(layer.target - layer.weight, -step, step);
    layer.time += deltaTime * layer.pace;
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

void DrivePose(const RagdollAsset& asset, RagdollInstance& instance, RagdollPose& pose, float deltaTime) {
    for (PoseLayer& layer : pose.layers) Advance(layer, deltaTime);

    for (std::size_t i = 0; i < asset.parts.size(); ++i) {
        const int index = asset.settings->GetConstraintIndexForBodyIndex(static_cast<int>(i));
        JPH::TwoBodyConstraint* constraint = index >= 0 ? instance.ragdoll->GetConstraint(index) : nullptr;
        if (constraint == nullptr || constraint->GetSubType() != JPH::EConstraintSubType::SwingTwist) continue;

        const PartId part{ static_cast<std::uint16_t>(i) };
        Drive(static_cast<JPH::SwingTwistConstraint&>(*constraint), Blend(asset, pose, part), asset.motorTorque * pose.strength);
    }
}

}
