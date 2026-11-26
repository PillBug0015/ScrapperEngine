#pragma once

#include "Scrapper/Net/NetTypes.h"

#include <entt/entity/registry.hpp>


namespace Scrapper {
// The peer that simulates a NetId entity and sends its pose; every other peer follows it as a replica.
// Set on the server (none = the server owns it) and mirrored to clients through snapshots.
// When a client leaves, what it owned goes back to the server.
struct NetOwner {
    ConnectionId connection = ServerOwner;
};

inline ConnectionId OwnerOf(const entt::registry& registry, entt::entity entity) {
    const NetOwner* owner = registry.try_get<NetOwner>(entity);
    return owner != nullptr ? owner->connection : ServerOwner;
}

}
