#pragma once

#include "RagdollBody.h"
#include "Scrapper/Jolt/RagdollPose.h"

#include <Jolt/Physics/Body/BodyInterface.h>


namespace Scrapper {
// One fixed step: layer weights ease to their targets, clips advance, and the joint motors take the blend.
void DrivePose(JPH::BodyInterface& bodies, const RagdollAsset& asset, RagdollInstance& instance, RagdollPose& pose, float deltaTime);

}
