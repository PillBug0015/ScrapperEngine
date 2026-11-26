#pragma once

#include "Scrapper/Jolt/Ragdoll.h"
#include "Scrapper/Net/NetSession.h"
#include "Scrapper/ScriptEngine/GameObject.h"

#include <array>
#include <cstdint>
#include <memory>


inline constexpr int CrewCount = 4; // slot 0 is the host's; the others go to clients as they join
using CrewList = std::array<Scrapper::GameObject, CrewCount>;

inline constexpr const char* HeadJoint = "Head";
inline constexpr Vector3 EyeFromHead = { 0.0f, 0.06f, -0.08f }; // character space, from the neck at rest

// Spring character on a ragdoll.
Scrapper::GameObject SpawnCrewMember(Scrapper::World& world, const std::shared_ptr<const Scrapper::RagdollAsset>& body,
                                     std::uint32_t netId, Vector3 feet, Color color);

// The head, which a first-person camera would otherwise see from the inside.
void SetHeadVisible(Scrapper::GameObject crew, bool visible);

// Server: hands the first free slot to a joining client. Slots return by themselves when it leaves.
void AssignCrew(const CrewList& crew, Scrapper::ConnectionId client);
// The member this peer controls: the first one it owns (the host never hands out slot 0).
Scrapper::GameObject FindLocalCrew(const CrewList& crew, const Scrapper::NetSession& session);
