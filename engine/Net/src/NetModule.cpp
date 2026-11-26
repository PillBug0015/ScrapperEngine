#include "Scrapper/Net/NetModule.h"
#include "Scrapper/Core/World.h"
#include "NetTransport.h"
#include "Replication.h"

#include <cstdio>
#include <vector>


namespace Scrapper {
// Systems with access to the session's internals.
struct NetSystems {
    static void Receive(World& world, float) {
        NetSession& session = world.Resource<NetSession>();
        std::vector<NetEvent> events;
        session.transport->Poll(events);

        for (NetEvent& event : events) {
            switch (event.type) {
                case NetEvent::Type::Connected:
                    if (session.IsServer()) {
                        ByteWriter welcome;
                        welcome.Write(event.connection);
                        session.Send(event.connection, NetChannel::Reliable, NetMessage::Welcome, welcome);
                        std::fprintf(stderr, "[Net] client %u connected\n", event.connection);
                    } else {
                        session.server = event.connection;
                        std::fprintf(stderr, "[Net] connected to %s\n", session.address.c_str());
                    }
                    if (session.onConnected) session.onConnected(event.connection);
                    break;

                case NetEvent::Type::Disconnected:
                    if (!session.IsServer()) {
                        if (session.server == 0) {
                            std::fprintf(stderr, "[Net] could not reach %s\n", session.address.c_str());
                            break;
                        }
                        session.server = 0;
                        session.localId = ServerOwner;
                    }
                    std::fprintf(stderr, "[Net] connection %u closed\n", event.connection);
                    if (session.onDisconnected) session.onDisconnected(event.connection);
                    ReplicationSystems::Forget(world, event.connection);
                    break;

                case NetEvent::Type::Received: {
                    ByteReader reader(event.payload.data(), event.payload.size());
                    MessageType type = 0;
                    if (!reader.Read(type)) break;

                    if (!session.IsServer() && type == NetMessage::Welcome) {
                        reader.Read(session.localId);
                    } else if (type == NetMessage::Snapshot) {
                        ReplicationSystems::Receive(world, event.connection, reader);
                    } else if (auto handler = session.handlers.find(type); handler != session.handlers.end()) {
                        handler->second(event.connection, reader);
                    }
                    break;
                }
            }
        }
    }

    static void Flush(World& world, float) {
        world.Resource<NetSession>().transport->Flush();
    }

    static void Start(NetSession& session, const NetConfig& config) {
        if (session.IsServer()) {
            if (session.transport->Listen(config.port, config.maxClients)) std::fprintf(stderr, "[Net] listening on port %u\n", config.port);
            else std::fprintf(stderr, "[Net] could not listen on port %u\n", config.port);
        } else if (session.transport->Connect(config.address, config.port)) {
            std::fprintf(stderr, "[Net] connecting to %s:%u\n", config.address.c_str(), config.port);
        } else {
            std::fprintf(stderr, "[Net] invalid server address %s\n", config.address.c_str());
        }
    }

    static void Stop(NetSession& session) {
        session.transport->Close();
    }
};

void NetModule::Install(World& world) {
    NetSession& session = world.AddResource<NetSession>(config.role, config.address, config.port);
    world.AddResource<Replication>();
    NetSystems::Start(session, config);

    Pipeline& pipeline = world.GetPipeline();
    pipeline.Add(NetSystem::Receive, &NetSystems::Receive, Placement::InPhase(Phase::Input));
    pipeline.Add(NetSystem::Authority, &ReplicationSystems::SyncAuthority, Placement::InPhase(Phase::Input).After(NetSystem::Receive));
    pipeline.Add(NetSystem::Interpolate, &ReplicationSystems::Interpolate, Placement::InPhase(Phase::Input).After(NetSystem::Authority));
    pipeline.Add(NetSystem::PlaceRiders, &ReplicationSystems::PlaceRiders,
                 Placement::InPhase(Phase::PostSimulate).Before(CoreSystem::PropagatePostSimulate));
    pipeline.Add(NetSystem::SendSnapshot, &ReplicationSystems::Send,
                 Placement::InPhase(Phase::PostSimulate).After(CoreSystem::PropagatePostSimulate));
    pipeline.Add(NetSystem::Flush, &NetSystems::Flush, Placement::InPhase(Phase::Cleanup));
}

void NetModule::Uninstall(World& world) {
    Pipeline& pipeline = world.GetPipeline();
    pipeline.Remove(NetSystem::Receive);
    pipeline.Remove(NetSystem::Authority);
    pipeline.Remove(NetSystem::Interpolate);
    pipeline.Remove(NetSystem::PlaceRiders);
    pipeline.Remove(NetSystem::SendSnapshot);
    pipeline.Remove(NetSystem::Flush);

    NetSystems::Stop(world.Resource<NetSession>());
    world.Registry().clear<NetReplica>();
    world.RemoveResource<Replication>();
    world.RemoveResource<NetSession>();
}

}
