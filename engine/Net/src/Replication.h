#pragma once

#include "Scrapper/Core/Math.h"
#include "Scrapper/Net/ByteStream.h"
#include "Scrapper/Net/NetTypes.h"

#include <entt/entity/entity.hpp>

#include <array>
#include <cstdint>
#include <unordered_map>


namespace Scrapper {
class World;

// Module resource: snapshot timing, and per sender the ordering and clock estimate.
struct Replication {
    struct Peer {
        std::uint32_t lastTick = 0;
        bool synced = false;
        double clockOffset = 0.0; // sender clock - local clock
    };
    std::unordered_map<ConnectionId, Peer> peers;
    float sendTimer = 0.0f;
    std::uint32_t tick = 0;
};

// Recent states of an entity someone else owns, oldest first, in the local clock.
// The pose is relative to `frame` (entt::null: the world), or the LocalTransform itself for a part.
struct NetReplica {
    struct State {
        double time = 0.0;
        entt::entity frame = entt::null;
        Vector3 position = { 0.0f, 0.0f, 0.0f };
        Quaternion rotation = { 0.0f, 0.0f, 0.0f, 1.0f };
    };
    std::array<State, 4> history = {};
    std::size_t count = 0;
    bool local = false; // a part: the states are its LocalTransform
    State shown;        // this frame's sample

    void Push(const State& state);
};

namespace ReplicationSystems {
void Receive(World& world, ConnectionId sender, ByteReader& reader);
void Forget(World& world, ConnectionId connection); // a peer left: drop its clock, take back what it owned
void SyncAuthority(World& world, float deltaTime);  // RemoteAuthority on exactly what this peer doesn't own
void Interpolate(World& world, float deltaTime);
void PlaceRiders(World& world, float deltaTime); // again after physics: the vehicle may have moved
void Send(World& world, float deltaTime);
}

}
