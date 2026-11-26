#pragma once

#include "Scrapper/Jolt/Ragdoll.h"

#include <memory>
#include <optional>


// The crew's ragdoll, seven chunks (pelvis, torso, head, two arms, two legs), its clips, and the ids
// the game needs from it.
struct CrewBody {
    std::shared_ptr<const Scrapper::RagdollAsset> asset;
    Scrapper::PartId head;
    Scrapper::ClipId idle;
    Scrapper::ClipId walk;
    Scrapper::ClipId air;
};

std::optional<CrewBody> MakeCrewBody();
