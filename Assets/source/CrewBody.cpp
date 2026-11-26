#include "CrewBody.h"

#include <cstdio>

using namespace Scrapper;

namespace {
namespace Part {
constexpr const char* Pelvis = "Pelvis";
constexpr const char* Torso = "Torso";
constexpr const char* Head = "Head";
constexpr const char* ArmL = "ArmL";
constexpr const char* ArmR = "ArmR";
constexpr const char* LegL = "LegL";
constexpr const char* LegR = "LegR";
}
namespace Clip {
constexpr const char* Idle = "idle";
constexpr const char* Walk = "walk";
constexpr const char* Air = "air";
}
namespace Mask {
constexpr const char* Body = "body";
constexpr const char* Upper = "upper";
constexpr const char* Lower = "lower";
}

// Character space: feet on the ground, facing -Z, right is +X. Each limb hangs from its joint at `from`.
RagdollDesc Body() {
    RagdollDesc body;
    body.parts = {
        { Part::Pelvis, "",          { 0.0f, 0.90f, 0.0f },    { 0.0f, 1.00f, 0.0f },    0.15f, 12.0f },
        { Part::Torso,  Part::Pelvis, { 0.0f, 1.02f, 0.0f },    { 0.0f, 1.34f, 0.0f },    0.18f, 24.0f, 35.0f, 30.0f },
        { Part::Head,   Part::Torso,  { 0.0f, 1.52f, 0.0f },    { 0.0f, 1.60f, 0.0f },    0.12f, 5.0f, 40.0f, 60.0f },
        { Part::ArmL,   Part::Torso,  { -0.25f, 1.32f, 0.0f },  { -0.29f, 0.82f, 0.0f },  0.065f, 4.0f, 110.0f, 45.0f },
        { Part::ArmR,   Part::Torso,  { 0.25f, 1.32f, 0.0f },   { 0.29f, 0.82f, 0.0f },   0.065f, 4.0f, 110.0f, 45.0f },
        { Part::LegL,   Part::Pelvis, { -0.11f, 0.90f, 0.0f },  { -0.11f, 0.10f, 0.0f },  0.09f, 9.0f, 70.0f, 15.0f },
        { Part::LegR,   Part::Pelvis, { 0.11f, 0.90f, 0.0f },   { 0.11f, 0.10f, 0.0f },   0.09f, 9.0f, 70.0f, 15.0f },
    };
    body.masks = {
        { Mask::Body, { Part::Pelvis } },
        { Mask::Upper, { Part::Torso } },
        { Mask::Lower, { Part::LegL, Part::LegR } },
    };
    return body;
}

// Degrees from rest, relative to the parent: +x swings a hanging limb forward, +z raises the right arm outward.
RagdollClip Idle() {
    return { Clip::Idle, 3.0f, true, {
        { Part::Torso, { { 0.0f, { 0, 0, 0 } }, { 1.5f, { 3, 0, 0 } }, { 3.0f, { 0, 0, 0 } } } },
        { Part::Head,  { { 0.0f, { 0, 0, 0 } }, { 1.5f, { -3, 0, 0 } }, { 3.0f, { 0, 0, 0 } } } },
        { Part::ArmL,  { { 0.0f, { 0, 0, -6 } }, { 1.5f, { 0, 0, -10 } }, { 3.0f, { 0, 0, -6 } } } },
        { Part::ArmR,  { { 0.0f, { 0, 0, 6 } }, { 1.5f, { 0, 0, 10 } }, { 3.0f, { 0, 0, 6 } } } },
    } };
}

RagdollClip Walk() {
    return { Clip::Walk, 1.0f, true, {
        { Part::Torso, { { 0.0f, { 5, 6, 0 } }, { 0.5f, { 5, -6, 0 } }, { 1.0f, { 5, 6, 0 } } } },
        { Part::ArmL,  { { 0.0f, { -25, 0, -6 } }, { 0.5f, { 25, 0, -6 } }, { 1.0f, { -25, 0, -6 } } } },
        { Part::ArmR,  { { 0.0f, { 25, 0, 6 } }, { 0.5f, { -25, 0, 6 } }, { 1.0f, { 25, 0, 6 } } } },
        { Part::LegL,  { { 0.0f, { 30, 0, 0 } }, { 0.5f, { -30, 0, 0 } }, { 1.0f, { 30, 0, 0 } } } },
        { Part::LegR,  { { 0.0f, { -30, 0, 0 } }, { 0.5f, { 30, 0, 0 } }, { 1.0f, { -30, 0, 0 } } } },
    }, 1.5f };
}

// Arms thrown up and flapping, legs pedalling.
RagdollClip Air() {
    return { Clip::Air, 0.6f, true, {
        { Part::Torso, { { 0.0f, { -8, 0, 0 } }, { 0.6f, { -8, 0, 0 } } } },
        { Part::ArmL,  { { 0.0f, { -20, 0, -70 } }, { 0.3f, { 20, 0, -95 } }, { 0.6f, { -20, 0, -70 } } } },
        { Part::ArmR,  { { 0.0f, { 20, 0, 95 } }, { 0.3f, { -20, 0, 70 } }, { 0.6f, { 20, 0, 95 } } } },
        { Part::LegL,  { { 0.0f, { 25, 0, 0 } }, { 0.3f, { 5, 0, 0 } }, { 0.6f, { 25, 0, 0 } } } },
        { Part::LegR,  { { 0.0f, { -10, 0, 0 } }, { 0.3f, { 15, 0, 0 } }, { 0.6f, { -10, 0, 0 } } } },
    } };
}

// Blended in order: idle underneath, walk over it by speed, air over both off the ground.
std::vector<PoseLayerDesc> Layers() {
    return {
        { Clip::Idle, Mask::Body, PoseDriver::Always },
        { Clip::Walk, Mask::Body, PoseDriver::Moving },
        { Clip::Air, Mask::Body, PoseDriver::Airborne, 1.0f / 6.0f },
    };
}
}

std::optional<CrewBody> MakeCrewBody() {
    std::shared_ptr<const RagdollAsset> asset = MakeRagdollAsset(Body(), { Idle(), Walk(), Air() });
    if (asset == nullptr) return std::nullopt;

    const std::optional<PartId> head = asset->FindPart(Part::Head);
    std::optional<std::vector<PoseLayer>> layers = ResolveLayers(*asset, Layers());
    if (!head || !layers) {
        std::fprintf(stderr, "[Game] crew body is missing a part or layer the game uses\n");
        return std::nullopt;
    }
    return CrewBody{ std::move(asset), *head, std::move(*layers) };
}
