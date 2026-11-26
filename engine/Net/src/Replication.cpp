#include "Replication.h"
#include "Scrapper/Net/NetId.h"
#include "Scrapper/Net/NetOwner.h"
#include "Scrapper/Net/NetSession.h"
#include "Scrapper/Core/Authority.h"
#include "Scrapper/Core/Lifecycle.h"
#include "Scrapper/Core/Parts.h"
#include "Scrapper/Core/StandingOn.h"
#include "Scrapper/Core/Transform.h"
#include "Scrapper/Core/World.h"

#include <algorithm>
#include <chrono>
#include <vector>


namespace Scrapper {
namespace {
constexpr float SendRate = 30.0f;
// ~2 send intervals: one late packet still leaves a state to interpolate towards.
constexpr double InterpolationDelay = 0.07;

double Now() {
    using Clock = std::chrono::steady_clock;
    static const Clock::time_point start = Clock::now();
    return std::chrono::duration<double>(Clock::now() - start).count();
}

// Snapshot wire format: [tick][sender clock][count] then `count` entries,
// each followed by [part count] and the LocalTransform poses of its Parts.
struct Entry {
    std::uint32_t id = 0;
    ConnectionId owner = ServerOwner;
    std::uint32_t frame = 0; // NetId the pose is relative to; 0: world space
    Vector3 position = { 0.0f, 0.0f, 0.0f };
    Quaternion rotation = { 0.0f, 0.0f, 0.0f, 1.0f };
};

struct LocalPose {
    Vector3 position = { 0.0f, 0.0f, 0.0f };
    Quaternion rotation = { 0.0f, 0.0f, 0.0f, 1.0f };
};

// An entity and its parts are one unit: same authority, same snapshot entry.
template <typename Fn>
void ForEntityAndParts(const entt::registry& registry, entt::entity entity, Fn&& fn) {
    fn(entity);
    if (const Parts* parts = registry.try_get<Parts>(entity)) {
        for (entt::entity part : parts->entities) fn(part);
    }
}

std::unordered_map<std::uint32_t, entt::entity> EntitiesById(const entt::registry& registry) {
    std::unordered_map<std::uint32_t, entt::entity> byId;
    for (auto [entity, id] : registry.view<NetId>().each()) byId.emplace(id.value, entity);
    return byId;
}

// Held at the oldest/newest state outside the buffered range; never extrapolated.
NetReplica::State Sample(const NetReplica& replica, double time) {
    const NetReplica::State* from = &replica.history[0];
    const NetReplica::State* to = &replica.history[0];
    for (std::size_t i = 0; i < replica.count; ++i) {
        if (replica.history[i].time <= time) {
            from = &replica.history[i];
            to = i + 1 < replica.count ? &replica.history[i + 1] : &replica.history[i];
        }
    }
    const double span = to->time - from->time;
    const float t = span > 0.0 ? Clamp(static_cast<float>((time - from->time) / span), 0.0f, 1.0f) : 0.0f;
    return { time, from->frame, Vector3Lerp(from->position, to->position, t), QuaternionSlerp(from->rotation, to->rotation, t) };
}
}

void NetReplica::Push(const State& state) {
    // A new frame makes the old states meaningless to blend with: snap.
    if (count > 0 && history[count - 1].frame != state.frame) count = 0;
    if (count == history.size()) {
        std::rotate(history.begin(), history.begin() + 1, history.end());
        --count;
    }
    history[count++] = state;
}

namespace ReplicationSystems {
void Receive(World& world, ConnectionId sender, ByteReader& reader) {
    std::uint32_t tick = 0;
    double sentAt = 0.0;
    std::uint16_t count = 0;
    if (!reader.Read(tick) || !reader.Read(sentAt) || !reader.Read(count)) return;

    Replication::Peer& peer = world.Resource<Replication>().peers[sender];
    if (tick <= peer.lastTick) return; // unreliable channel: late or duplicated packet
    peer.lastTick = tick;

    // Smoothed so one delayed packet doesn't jerk the whole timeline.
    const double offsetSample = sentAt - Now();
    peer.clockOffset = peer.synced ? peer.clockOffset + (offsetSample - peer.clockOffset) * 0.05 : offsetSample;
    peer.synced = true;
    const double localTime = sentAt - peer.clockOffset;

    NetSession& session = world.Resource<NetSession>();
    entt::registry& registry = world.Registry();
    const auto byId = EntitiesById(registry);
    const auto find = [&byId](std::uint32_t id) {
        const auto it = byId.find(id);
        return it != byId.end() ? it->second : entt::entity{ entt::null };
    };

    std::vector<LocalPose> partPoses;
    for (std::uint16_t i = 0; i < count; ++i) {
        Entry entry;
        std::uint8_t partCount = 0;
        if (!reader.Read(entry) || !reader.Read(partCount)) return;
        partPoses.resize(partCount);
        for (LocalPose& pose : partPoses) {
            if (!reader.Read(pose)) return;
        }

        const entt::entity entity = find(entry.id);
        if (entity == entt::null) continue;
        if (session.IsServer()) {
            if (OwnerOf(registry, entity) != sender) continue; // a client speaks only for what it owns
        } else if (OwnerOf(registry, entity) != entry.owner) {
            registry.emplace_or_replace<NetOwner>(entity, entry.owner);
        }
        if (session.Owns(OwnerOf(registry, entity))) continue; // ours: we simulate it

        const entt::entity frame = entry.frame != 0 ? find(entry.frame) : entt::entity{ entt::null };
        if (entry.frame != 0 && frame == entt::null) continue;
        registry.emplace_or_replace<StandingOn>(entity, frame);
        registry.get_or_emplace<NetReplica>(entity).Push({ localTime, frame, entry.position, entry.rotation });

        const Parts* parts = registry.try_get<Parts>(entity);
        if (parts == nullptr || parts->entities.size() != partPoses.size()) continue;
        for (std::size_t p = 0; p < partPoses.size(); ++p) {
            NetReplica& replica = registry.get_or_emplace<NetReplica>(parts->entities[p]);
            replica.local = true;
            replica.Push({ localTime, entt::null, partPoses[p].position, partPoses[p].rotation });
        }
    }
}

void Forget(World& world, ConnectionId connection) {
    world.Resource<Replication>().peers.erase(connection);
    if (!world.Resource<NetSession>().IsServer()) return;

    entt::registry& registry = world.Registry();
    std::vector<entt::entity> owned;
    for (auto [entity, owner] : registry.view<NetOwner>().each()) {
        if (owner.connection == connection) owned.push_back(entity);
    }
    registry.remove<NetOwner>(owned.begin(), owned.end());
}

void SyncAuthority(World& world, float) {
    const NetSession& session = world.Resource<NetSession>();
    entt::registry& registry = world.Registry();
    for (entt::entity entity : registry.view<NetId>()) {
        const bool local = session.Owns(OwnerOf(registry, entity));
        ForEntityAndParts(registry, entity, [&](entt::entity e) {
            const bool remote = registry.all_of<RemoteAuthority>(e);
            if (local && remote) {
                registry.remove<RemoteAuthority>(e);
                registry.remove<NetReplica>(e);
            } else if (!local && !remote) {
                registry.emplace<RemoteAuthority>(e);
            }
        });
    }
}

// Replicas are shown slightly in the past, so there is always a later state to blend towards.
void Interpolate(World& world, float deltaTime) {
    entt::registry& registry = world.Registry();
    const double renderTime = Now() - InterpolationDelay;
    for (auto [entity, replica] : registry.view<NetReplica>().each()) {
        if (replica.count == 0) continue;
        replica.shown = Sample(replica, renderTime);
        if (replica.local) {
            LocalTransform& local = registry.get<LocalTransform>(entity);
            local.position = replica.shown.position;
            local.rotation = replica.shown.rotation;
        } else if (replica.shown.frame == entt::null) {
            Transforms::SetWorldPose(registry, entity, replica.shown.position, replica.shown.rotation);
        }
    }
    PlaceRiders(world, deltaTime);
}

// Riders go after everything else, so they sit on their vehicle's current pose.
void PlaceRiders(World& world, float) {
    entt::registry& registry = world.Registry();
    for (auto [entity, replica] : registry.view<NetReplica>().each()) {
        const NetReplica::State& state = replica.shown;
        if (replica.count == 0 || state.frame == entt::null || !registry.valid(state.frame)) continue;
        const WorldTransform frame = Transforms::ComputeWorld(registry, state.frame);
        Transforms::SetWorldPose(registry, entity,
                                 Vector3Add(frame.position, Vector3RotateByQuaternion(state.position, frame.rotation)),
                                 QuaternionMultiply(frame.rotation, state.rotation));
    }
}

// A client sends what it owns; the server sends everything, relaying what clients own.
void Send(World& world, float deltaTime) {
    Replication& replication = world.Resource<Replication>();
    NetSession& session = world.Resource<NetSession>();
    const float interval = 1.0f / SendRate;
    replication.sendTimer += deltaTime;
    if (replication.sendTimer < interval) return;
    replication.sendTimer = replication.sendTimer >= 2.0f * interval ? 0.0f : replication.sendTimer - interval;
    if (session.IsServer() ? session.ClientCount() == 0 : !session.IsConnected()) return;

    entt::registry& registry = world.Registry();
    std::vector<entt::entity> sent;
    for (entt::entity entity : registry.view<NetId, WorldTransform>(entt::exclude<Disabled>)) {
        if (session.IsServer() || session.Owns(OwnerOf(registry, entity))) sent.push_back(entity);
    }

    ByteWriter snapshot;
    snapshot.Write(++replication.tick);
    snapshot.Write(Now());
    snapshot.Write(static_cast<std::uint16_t>(sent.size()));
    for (entt::entity entity : sent) {
        const WorldTransform& pose = registry.get<WorldTransform>(entity);
        Entry entry{ registry.get<NetId>(entity).value, OwnerOf(registry, entity), 0, pose.position, pose.rotation };
        const StandingOn* on = registry.try_get<StandingOn>(entity);
        if (on != nullptr && registry.valid(on->entity) && registry.all_of<NetId, WorldTransform>(on->entity)) {
            const WorldTransform& frame = registry.get<WorldTransform>(on->entity);
            const Quaternion inverse = QuaternionInvert(frame.rotation);
            entry.frame = registry.get<NetId>(on->entity).value;
            entry.position = Vector3RotateByQuaternion(Vector3Subtract(pose.position, frame.position), inverse);
            entry.rotation = QuaternionMultiply(inverse, pose.rotation);
        }
        snapshot.Write(entry);

        const Parts* parts = registry.try_get<Parts>(entity);
        const std::size_t partCount = parts != nullptr && parts->entities.size() <= 255 ? parts->entities.size() : 0;
        snapshot.Write(static_cast<std::uint8_t>(partCount));
        for (std::size_t p = 0; p < partCount; ++p) {
            const LocalTransform& local = registry.get<LocalTransform>(parts->entities[p]);
            snapshot.Write(LocalPose{ local.position, local.rotation });
        }
    }

    if (session.IsServer()) session.Broadcast(NetChannel::Unreliable, NetMessage::Snapshot, snapshot);
    else session.SendToServer(NetChannel::Unreliable, NetMessage::Snapshot, snapshot);
}
}

}
