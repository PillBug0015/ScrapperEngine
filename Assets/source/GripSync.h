#pragma once

#include "Scrapper/Net/NetTypes.h"
#include "Scrapper/ScriptEngine/GameObject.h"


// A grip is decided by the peer that owns the hand and mirrored on every peer, so each one pulls the
// side it simulates. Grips on things that aren't replicated (the static ground) stay with their owner.
void SendGrip(Scrapper::World& world, entt::entity hand);         // its Grip, or that it let go
void SendAllGrips(Scrapper::World& world, Scrapper::ConnectionId to); // server: a joiner catches up
void ReleaseGripsOf(Scrapper::World& world, Scrapper::ConnectionId client); // server: it left
void ListenForGrips(Scrapper::World& world);                      // apply what arrives; the server also relays
