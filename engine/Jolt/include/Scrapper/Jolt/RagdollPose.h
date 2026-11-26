#pragma once

#include "Scrapper/Jolt/Ragdoll.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>


namespace Scrapper {
// What sets a layer's target weight and its pace (clip seconds per second) each step.
enum class PoseDriver : std::uint8_t {
    Manual,   // game code writes target and pace
    Always,   // weight 1, plays in seconds
    Moving,   // weight and pace follow the ground speed against the clip's stride
    Airborne, // weight 1 off the ground, plays in seconds
};

// Authored by name; ResolveLayers turns it into a PoseLayer.
struct PoseLayerDesc {
    std::string clip;
    std::string mask;
    PoseDriver driver = PoseDriver::Always;
    float fadeSeconds = 0.0f; // how long the weight takes from 0 to 1
};

struct PoseLayer {
    ClipId clip;
    MaskId mask;
    PoseDriver driver = PoseDriver::Always;
    float fadeSeconds = 0.0f;
    float target = 0.0f;
    float pace = 1.0f;
    float weight = 0.0f; // eases towards target
    float time = 0.0f;   // in the clip
};

// Points one direction of a part (axis, in its rest frame) at a world point: reaching for something,
// looking at it. Applied after the layers, so it overrides them by its weight.
struct PoseAim {
    PartId part;
    Vector3 axis = { 0.0f, -1.0f, 0.0f };
    float fadeSeconds = 0.0f;
    Vector3 point = { 0.0f, 0.0f, 0.0f }; // world space; game code writes point and target
    float target = 0.0f;
    float weight = 0.0f; // eases towards target
};

// What a ragdoll's motors chase: the clip layers blended in order over its rest pose, then the aims.
struct RagdollPose {
    std::vector<PoseLayer> layers;
    std::vector<PoseAim> aims; // at most one per part
    float strength = 1.0f;     // scales the asset's motor torque; a character writes its stability

    PoseAim* Aim(PartId part) {
        for (PoseAim& aim : aims) {
            if (aim.part == part) return &aim;
        }
        return nullptr;
    }
};

// Empty (and a log line) when a layer names a clip or mask the asset doesn't have.
std::optional<std::vector<PoseLayer>> ResolveLayers(const RagdollAsset& asset, const std::vector<PoseLayerDesc>& layers);

}
