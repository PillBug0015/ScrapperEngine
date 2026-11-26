#pragma once

#include <Jolt/Jolt.h>
#include <entt/entity/entity.hpp>


namespace Scrapper {
// Every body's user data is the entity that owns it.
inline JPH::uint64 ToUserData(entt::entity entity) {
    return static_cast<JPH::uint64>(entt::to_integral(entity));
}

inline entt::entity ToEntity(JPH::uint64 userData) {
    return static_cast<entt::entity>(static_cast<entt::id_type>(userData));
}

}
