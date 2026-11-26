#pragma once

#include <entt/entity/entity.hpp>


namespace Scrapper {
// What an entity rests on; entt::null is the static world. Written by whoever simulates it.
// Poses are replicated relative to it, so riders stay put on a moving vehicle despite latency.
struct StandingOn {
    entt::entity entity = entt::null;
};

}
