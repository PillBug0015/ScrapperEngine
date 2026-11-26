#pragma once

#include "Scrapper/Net/NetTypes.h"


// Poses travel in snapshots; these are what can't: requests to the server, and grips everyone mirrors.
namespace GameMessages {
inline constexpr Scrapper::MessageType Shove = Scrapper::NetMessage::FirstGameMessage;     // client -> server: throw the tank up
inline constexpr Scrapper::MessageType Drive = Scrapper::NetMessage::FirstGameMessage + 1; // client -> server: Vector2 x turn, y throttle
inline constexpr Scrapper::MessageType Grip = Scrapper::NetMessage::FirstGameMessage + 2;  // either way: a hand's grip, see GripSync
}
