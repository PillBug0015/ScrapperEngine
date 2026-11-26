#pragma once

#include <entt/entity/registry.hpp>

#include <cstdint>
#include <optional>


namespace Scrapper {
// An entity named so that every peer finds the same one: a NetId entity, or one of its Parts.
struct NetRef {
    static constexpr std::uint16_t Whole = 0xFFFF;

    std::uint32_t id = 0;
    std::uint16_t part = Whole;
};

// Empty when the entity isn't replicated (no NetId, and not a part of something that has one).
std::optional<NetRef> ToNetRef(const entt::registry& registry, entt::entity entity);
// entt::null when this peer has no such entity.
entt::entity FromNetRef(const entt::registry& registry, NetRef ref);

}
