#pragma once

#include <entt/entity/entity.hpp>

#include <vector>


namespace Scrapper {
// Children that move on their own but belong to this entity, in a fixed order (a ragdoll's limbs).
// They share its authority and are replicated with it, as local poses.
struct Parts {
    std::vector<entt::entity> entities;
};

}
