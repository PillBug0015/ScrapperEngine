#pragma once

#include <entt/entity/registry.hpp>


namespace Scrapper {
struct Inactive {};       // switched off by game code (SetActive(false))
struct Disabled {};       // effective state: this or an ancestor is Inactive. Modules skip Disabled entities.
struct PendingDestroy {}; // removed in Phase::Cleanup, so nothing disappears in the middle of a frame

namespace Lifecycle {
void SetActive(entt::registry& registry, entt::entity entity, bool active);
bool IsActiveSelf(const entt::registry& registry, entt::entity entity);
bool IsActiveInHierarchy(const entt::registry& registry, entt::entity entity); // walks the chain: always current

// Marks the entity and its descendants; they are destroyed at the end of the frame.
void Destroy(entt::registry& registry, entt::entity entity);
}

}
