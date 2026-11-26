#pragma once

#include <cstddef>
#include <cstdint>


namespace Scrapper {
using ConnectionId = std::uint32_t; // remote peer, assigned by the transport (0 = none)
inline constexpr ConnectionId ServerOwner = 0; // as an owner, 0 is the server itself
using MessageType = std::uint16_t;

enum class NetChannel : std::uint8_t {
    Reliable = 0,   // ordered and guaranteed: joins, events, commands
    Unreliable = 1, // latest wins: snapshots, continuous input
};
inline constexpr std::size_t NetChannelCount = 2;

namespace NetMessage {
inline constexpr MessageType Welcome = 1;  // server -> client: your ConnectionId
inline constexpr MessageType Snapshot = 2; // poses of what the sender owns (the server relays everyone's)
inline constexpr MessageType FirstGameMessage = 100;
}

inline constexpr std::uint16_t DefaultPort = 47777;

}
