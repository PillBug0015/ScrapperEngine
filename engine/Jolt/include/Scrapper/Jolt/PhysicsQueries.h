#pragma once

#include "Scrapper/Core/Math.h"

#include <entt/entity/entity.hpp>

#include <optional>


namespace Scrapper {
class World;

struct Touch {
    entt::entity entity = entt::null; // whose body it is
    Vector3 point = { 0.0f, 0.0f, 0.0f };
};

// The body most inside a small sphere, skipping the given entity's own ragdoll parts (or its own body).
std::optional<Touch> FindTouch(World& world, Vector3 center, float radius, entt::entity ignore);

}
