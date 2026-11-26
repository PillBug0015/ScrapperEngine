#include "GripSync.h"
#include "GameMessages.h"
#include "Scrapper/Jolt/Grip.h"
#include "Scrapper/Net/NetOwner.h"
#include "Scrapper/Net/NetRef.h"
#include "Scrapper/Net/NetSession.h"

#include <optional>
#include <vector>

using namespace Scrapper;

namespace {
void WriteRef(ByteWriter& out, NetRef ref) {
    out.Write(ref.id);
    out.Write(ref.part);
}

bool ReadRef(ByteReader& in, NetRef& ref) {
    return in.Read(ref.id) && in.Read(ref.part);
}

// [hand][holding] then, when holding, [target][anchor][target anchor][strength].
std::optional<ByteWriter> Describe(const entt::registry& registry, entt::entity hand) {
    const std::optional<NetRef> handRef = ToNetRef(registry, hand);
    if (!handRef) return std::nullopt;
    const Grip* grip = registry.try_get<Grip>(hand);
    const std::optional<NetRef> targetRef = grip != nullptr ? ToNetRef(registry, grip->target) : std::nullopt;

    ByteWriter out;
    WriteRef(out, *handRef);
    out.Write(static_cast<std::uint8_t>(targetRef.has_value()));
    if (targetRef) {
        WriteRef(out, *targetRef);
        out.Write(grip->anchor);
        out.Write(grip->targetAnchor);
        out.Write(grip->strength);
    }
    return out;
}

void Publish(NetSession& session, const ByteWriter& message) {
    if (session.IsServer()) session.Broadcast(NetChannel::Reliable, GameMessages::Grip, message);
    else session.SendToServer(NetChannel::Reliable, GameMessages::Grip, message);
}

// The server takes a grip only from the hand's owner, then passes it on to everyone.
void Receive(World& world, ConnectionId sender, ByteReader& in) {
    entt::registry& registry = world.Registry();
    NetSession& session = world.Resource<NetSession>();
    NetRef handRef;
    std::uint8_t holding = 0;
    if (!ReadRef(in, handRef) || !in.Read(holding)) return;

    const entt::entity hand = FromNetRef(registry, handRef);
    const entt::entity owner = FromNetRef(registry, NetRef{ handRef.id, NetRef::Whole });
    if (hand == entt::null || (session.IsServer() && OwnerOf(registry, owner) != sender)) return;

    Grip grip;
    NetRef targetRef;
    if (holding != 0 && !(ReadRef(in, targetRef) && in.Read(grip.anchor) && in.Read(grip.targetAnchor) && in.Read(grip.strength))) return;
    grip.target = holding != 0 ? FromNetRef(registry, targetRef) : entt::entity{ entt::null };
    if (grip.target != entt::null) registry.emplace_or_replace<Grip>(hand, grip);
    else registry.remove<Grip>(hand);

    if (session.IsServer()) SendGrip(world, hand);
}
}

void SendGrip(World& world, entt::entity hand) {
    if (const std::optional<ByteWriter> message = Describe(world.Registry(), hand)) Publish(world.Resource<NetSession>(), *message);
}

void SendAllGrips(World& world, ConnectionId to) {
    for (entt::entity hand : world.Registry().view<Grip>()) {
        if (const std::optional<ByteWriter> message = Describe(world.Registry(), hand)) {
            world.Resource<NetSession>().Send(to, NetChannel::Reliable, GameMessages::Grip, *message);
        }
    }
}

void ReleaseGripsOf(World& world, ConnectionId client) {
    entt::registry& registry = world.Registry();
    std::vector<entt::entity> released;
    for (entt::entity hand : registry.view<Grip>()) {
        const std::optional<NetRef> ref = ToNetRef(registry, hand);
        if (ref && OwnerOf(registry, FromNetRef(registry, NetRef{ ref->id, NetRef::Whole })) == client) released.push_back(hand);
    }
    for (entt::entity hand : released) {
        registry.remove<Grip>(hand);
        SendGrip(world, hand);
    }
}

void ListenForGrips(World& world) {
    world.Resource<NetSession>().On(GameMessages::Grip, [&world](ConnectionId sender, ByteReader& in) { Receive(world, sender, in); });
}
