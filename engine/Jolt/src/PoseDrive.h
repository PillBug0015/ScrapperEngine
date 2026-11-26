#pragma once

#include "RagdollBody.h"
#include "Scrapper/Jolt/RagdollPose.h"


namespace Scrapper {
// One fixed step: layer weights ease to their targets, clips advance, and the joint motors take the blend.
void DrivePose(const RagdollAsset& asset, RagdollInstance& instance, RagdollPose& pose, float deltaTime);

}
