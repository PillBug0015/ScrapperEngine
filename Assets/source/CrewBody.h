#pragma once

#include "Scrapper/Jolt/Ragdoll.h"

#include <memory>


// The crew's ragdoll, seven chunks (pelvis, torso, head, two arms, two legs), and its clips: idle, walk, air.
std::shared_ptr<const Scrapper::RagdollAsset> MakeCrewBody();
