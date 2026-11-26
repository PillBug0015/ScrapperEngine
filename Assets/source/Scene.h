#pragma once

#include "Crew.h"

#include <optional>


// The sandbox: ground, crates, a ball, the tank and its crew. Host and clients build the same scene;
// the network module pairs the copies up by NetId.
struct Scene {
    Scrapper::GameObject tank;
    CrewList crew;

    static std::optional<Scene> Build(Scrapper::World& world); // empty when an asset is missing
};
