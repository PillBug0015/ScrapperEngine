#pragma once

#include <entt/entity/registry.hpp>

#include <vector>


namespace Scrapper {
// Cached results derived from Parent / Inactive, rebuilt only when those change.
struct HierarchyState {
    std::vector<entt::entity> order; // every LocalTransform entity, parents before children
    bool orderDirty = true;
    bool activityDirty = true;
};

void PropagateTransforms(entt::registry& registry);

}
