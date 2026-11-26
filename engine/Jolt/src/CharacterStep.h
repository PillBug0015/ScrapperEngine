#pragma once

#include "RagdollBody.h"
#include "Scrapper/Core/StandingOn.h"
#include "Scrapper/Jolt/RagdollPose.h"
#include "Scrapper/Jolt/SpringCharacter.h"


namespace Scrapper {
// One fixed step of the spring controller on its ragdoll: stuns, leg, movement, upright.
// Returns the speed over the ground.
float StepCharacter(JoltWorld& jolt, const CharacterSpringScale& scale, const RagdollAsset& asset, RagdollInstance& instance,
                    SpringCharacter& character, StandingOn& standingOn, float deltaTime);
// The layers the character drives get their targets and pace; its stability becomes the pose's strength.
void FeedPose(RagdollPose& pose, const RagdollAsset& asset, const SpringCharacter& character, float groundSpeed);

}
