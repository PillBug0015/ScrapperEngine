#include "Scrapper/Core/Lifecycle.h"
#include "Scrapper/Core/Transform.h"
#include "HierarchyState.h"


namespace Scrapper {
namespace Lifecycle {
void SetActive(entt::registry& registry, entt::entity entity, bool active) {
    if (active == !registry.all_of<Inactive>(entity)) {
        return;
    }
    if (active) registry.remove<Inactive>(entity);
    else registry.emplace<Inactive>(entity);
}

bool IsActiveSelf(const entt::registry& registry, entt::entity entity) {
    return !registry.all_of<Inactive>(entity);
}

bool IsActiveInHierarchy(const entt::registry& registry, entt::entity entity) {
    for (entt::entity e = entity; e != entt::null; e = Transforms::GetParent(registry, e)) {
        if (registry.all_of<Inactive>(e)) return false;
    }
    return true;
}

void Destroy(entt::registry& registry, entt::entity entity) {
    std::vector<entt::entity> pending{ entity };
    while (!pending.empty()) {
        const entt::entity current = pending.back();
        pending.pop_back();
        if (!registry.valid(current) || registry.all_of<PendingDestroy>(current)) continue;

        registry.emplace<PendingDestroy>(current);
        for (entt::entity child : Transforms::Children(registry, current)) pending.push_back(child);
    }
}
}

}
