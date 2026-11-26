#pragma once

#include "RagdollBody.h"
#include "Scrapper/Core/StandingOn.h"
#include "Scrapper/Jolt/SpringCharacter.h"


namespace Scrapper {
// One fixed step of the spring controller on its ragdoll: stuns, leg, movement, upright, limb pose.
void StepCharacter(JoltWorld& jolt, const CharacterSpringScale& scale, const RagdollAsset& asset, RagdollInstance& instance,
                   SpringCharacter& character, StandingOn& standingOn, float deltaTime);

}
