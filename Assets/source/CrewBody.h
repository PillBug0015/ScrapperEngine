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
    Scrapper::PartId leftArm;
    Scrapper::PartId rightArm;
    std::vector<Scrapper::PoseLayer> layers;
    std::vector<Scrapper::PoseAim> aims; // head looks forward, arms reach along their length
};

std::optional<CrewBody> MakeCrewBody();
