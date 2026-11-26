#pragma once

#include "Scrapper/Core/Math.h"

#include <string>
#include <vector>


namespace Scrapper {
// A pose over time for a ragdoll's joint motors: each part's rotation from rest, relative to its parent,
// in degrees around the character's axes (for a limb hanging down, +x swings it forward).
// Keys are eased into each other; parts without a track stay at rest. The root part has no joint.
struct RagdollClip {
    struct Key {
        float time = 0.0f;
        Vector3 degrees = { 0.0f, 0.0f, 0.0f };
    };
    struct Track {
        std::string part;
        std::vector<Key> keys; // by time; a looping clip repeats its first key at the end
    };

    std::string name;
    float duration = 1.0f;
    bool loop = true;
    std::vector<Track> tracks;
    float stride = 0.0f; // meters walked per loop, for layers paced by movement
};

}
