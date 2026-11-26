#pragma once

#include "CrewBody.h"
#include "Scrapper/Net/NetSession.h"
#include "Scrapper/ScriptEngine/GameObject.h"

#include <array>
#include <cstdint>


inline constexpr int CrewCount = 4; // slot 0 is the host's; the others go to clients as they join
using CrewList = std::array<Scrapper::GameObject, CrewCount>;

inline constexpr Vector3 EyeFromHead = { 0.0f, 0.06f, -0.08f }; // character space, from the neck at rest

// On every crew member: the parts the game reaches for by name, resolved once.
struct CrewRig {
    Scrapper::PartId head;
    Scrapper::PartId leftArm;
    Scrapper::PartId rightArm;
    entt::entity headMesh = entt::null; // hidden for its own first-person view
};

// Spring character on the crew's ragdoll.
Scrapper::GameObject SpawnCrewMember(Scrapper::World& world, const CrewBody& body, std::uint32_t netId, Vector3 feet, Color color);
void SetHeadVisible(Scrapper::GameObject crew, bool visible);

// Server: hands the first free slot to a joining client. Slots return by themselves when it leaves.
void AssignCrew(const CrewList& crew, Scrapper::ConnectionId client);
// The member this peer controls: the first one it owns (the host never hands out slot 0).
Scrapper::GameObject FindLocalCrew(const CrewList& crew, const Scrapper::NetSession& session);
