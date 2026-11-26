#include "Scrapper/Net/NetRef.h"
#include "Scrapper/Net/NetId.h"
#include "Scrapper/Core/Parts.h"
#include "Scrapper/Core/Transform.h"

#include <algorithm>


namespace Scrapper {
std::optional<NetRef> ToNetRef(const entt::registry& registry, entt::entity entity) {
    if (const NetId* id = registry.try_get<NetId>(entity)) return NetRef{ id->value, NetRef::Whole };

    const entt::entity whole = Transforms::GetParent(registry, entity);
    const NetId* id = whole != entt::null ? registry.try_get<NetId>(whole) : nullptr;
    const Parts* parts = whole != entt::null ? registry.try_get<Parts>(whole) : nullptr;
    if (id == nullptr || parts == nullptr) return std::nullopt;

    const auto it = std::find(parts->entities.begin(), parts->entities.end(), entity);
    if (it == parts->entities.end()) return std::nullopt;
    return NetRef{ id->value, static_cast<std::uint16_t>(it - parts->entities.begin()) };
}

entt::entity FromNetRef(const entt::registry& registry, NetRef ref) {
    for (auto [entity, id] : registry.view<NetId>().each()) {
        if (id.value != ref.id) continue;
        if (ref.part == NetRef::Whole) return entity;
        const Parts* parts = registry.try_get<Parts>(entity);
        return parts != nullptr && ref.part < parts->entities.size() ? parts->entities[ref.part] : entt::entity{ entt::null };
    }
    return entt::null;
}

}
