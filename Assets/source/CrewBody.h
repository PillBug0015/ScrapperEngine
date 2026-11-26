#pragma once

#include "Scrapper/Jolt/Ragdoll.h"
#include "Scrapper/Jolt/RagdollPose.h"

#include <memory>
#include <optional>
#include <vector>


// The crew's ragdoll, seven chunks (pelvis, torso, head, two arms, two legs), its clips and layers,
// and the ids the game needs from it.
struct CrewBody {
    std::shared_ptr<const Scrapper::RagdollAsset> asset;
    Scrapper::PartId head;
    std::vector<Scrapper::PoseLayer> layers;
};

std::optional<CrewBody> MakeCrewBody();
