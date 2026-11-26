#pragma once

#include "Scrapper/Core/Math.h"

#include <entt/entity/entity.hpp>


namespace Scrapper {
// Holds a point of this entity's body to a point of another entity's body, like a hand: it pulls with
// at most `strength`, so a light thing follows and a heavy one stays put while the holder hangs on.
// The Jolt module keeps a joint for it while both bodies are in the world.
struct Grip {
    entt::entity target = entt::null;
    Vector3 anchor = { 0.0f, 0.0f, 0.0f };       // on this body, in body space
    Vector3 targetAnchor = { 0.0f, 0.0f, 0.0f }; // on the target's body, in body space
    float strength = 600.0f;                     // newtons
};

}
