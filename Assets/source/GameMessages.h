#pragma once

#include "Scrapper/Net/NetTypes.h"


// Client -> server requests. Poses travel in snapshots; these are only what a client can't do itself.
namespace GameMessages {
inline constexpr Scrapper::MessageType Shove = Scrapper::NetMessage::FirstGameMessage; // throw the tank up
inline constexpr Scrapper::MessageType Drive = Scrapper::NetMessage::FirstGameMessage + 1; // Vector2: x turn, y throttle
}
