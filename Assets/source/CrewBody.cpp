#include "CrewBody.h"

using namespace Scrapper;

namespace {
// Character space: feet on the ground, facing -Z, right is +X. Each limb hangs from its joint at `from`.
RagdollDesc Body() {
    RagdollDesc body;
    body.parts = {
        { "Pelvis", "",       { 0.0f, 0.90f, 0.0f },    { 0.0f, 1.00f, 0.0f },    0.15f, 12.0f },
        { "Torso",  "Pelvis", { 0.0f, 1.02f, 0.0f },    { 0.0f, 1.34f, 0.0f },    0.18f, 24.0f, 35.0f, 30.0f },
        { "Head",   "Torso",  { 0.0f, 1.52f, 0.0f },    { 0.0f, 1.60f, 0.0f },    0.12f, 5.0f, 40.0f, 60.0f },
        { "ArmL",   "Torso",  { -0.25f, 1.32f, 0.0f },  { -0.29f, 0.82f, 0.0f },  0.065f, 4.0f, 110.0f, 45.0f },
        { "ArmR",   "Torso",  { 0.25f, 1.32f, 0.0f },   { 0.29f, 0.82f, 0.0f },   0.065f, 4.0f, 110.0f, 45.0f },
        { "LegL",   "Pelvis", { -0.11f, 0.90f, 0.0f },  { -0.11f, 0.10f, 0.0f },  0.09f, 9.0f, 70.0f, 15.0f },
        { "LegR",   "Pelvis", { 0.11f, 0.90f, 0.0f },   { 0.11f, 0.10f, 0.0f },   0.09f, 9.0f, 70.0f, 15.0f },
    };
    return body;
}

// Degrees from rest, relative to the parent: +x swings a hanging limb forward, +z raises the right arm outward.
RagdollClip Idle() {
    return { "idle", 3.0f, true, {
        { "Torso", { { 0.0f, { 0, 0, 0 } }, { 1.5f, { 3, 0, 0 } }, { 3.0f, { 0, 0, 0 } } } },
        { "Head",  { { 0.0f, { 0, 0, 0 } }, { 1.5f, { -3, 0, 0 } }, { 3.0f, { 0, 0, 0 } } } },
        { "ArmL",  { { 0.0f, { 0, 0, -6 } }, { 1.5f, { 0, 0, -10 } }, { 3.0f, { 0, 0, -6 } } } },
        { "ArmR",  { { 0.0f, { 0, 0, 6 } }, { 1.5f, { 0, 0, 10 } }, { 3.0f, { 0, 0, 6 } } } },
    } };
}

RagdollClip Walk() {
    return { "walk", 1.0f, true, {
        { "Torso", { { 0.0f, { 5, 6, 0 } }, { 0.5f, { 5, -6, 0 } }, { 1.0f, { 5, 6, 0 } } } },
        { "ArmL",  { { 0.0f, { -25, 0, -6 } }, { 0.5f, { 25, 0, -6 } }, { 1.0f, { -25, 0, -6 } } } },
        { "ArmR",  { { 0.0f, { 25, 0, 6 } }, { 0.5f, { -25, 0, 6 } }, { 1.0f, { 25, 0, 6 } } } },
        { "LegL",  { { 0.0f, { 30, 0, 0 } }, { 0.5f, { -30, 0, 0 } }, { 1.0f, { 30, 0, 0 } } } },
        { "LegR",  { { 0.0f, { -30, 0, 0 } }, { 0.5f, { 30, 0, 0 } }, { 1.0f, { -30, 0, 0 } } } },
    } };
}

// Arms thrown up and flapping, legs pedalling.
RagdollClip Air() {
    return { "air", 0.6f, true, {
        { "Torso", { { 0.0f, { -8, 0, 0 } }, { 0.6f, { -8, 0, 0 } } } },
        { "ArmL",  { { 0.0f, { -20, 0, -70 } }, { 0.3f, { 20, 0, -95 } }, { 0.6f, { -20, 0, -70 } } } },
        { "ArmR",  { { 0.0f, { 20, 0, 95 } }, { 0.3f, { -20, 0, 70 } }, { 0.6f, { 20, 0, 95 } } } },
        { "LegL",  { { 0.0f, { 25, 0, 0 } }, { 0.3f, { 5, 0, 0 } }, { 0.6f, { 25, 0, 0 } } } },
        { "LegR",  { { 0.0f, { -10, 0, 0 } }, { 0.3f, { 15, 0, 0 } }, { 0.6f, { -10, 0, 0 } } } },
    } };
}
}

std::shared_ptr<const RagdollAsset> MakeCrewBody() {
    return MakeRagdollAsset(Body(), { Idle(), Walk(), Air() });
}
