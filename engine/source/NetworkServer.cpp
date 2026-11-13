#include "Network/NetworkServer.h"
#include "Network/NetworkIdentity.h"
#include "Core/GameObject.h"
#include "Core/TransformSystem/Transform3D.h"

#include <raylib.h>


namespace ScrapperEngine {
void NetworkServer::Start() {
    listening = transport.Listen(port, maxClients);
    if (listening) {
        TraceLog(LOG_INFO, "NET: server listening on port %u", port);
    } else {
        TraceLog(LOG_WARNING, "NET: could not listen on port %u", port);
    }
}

void NetworkServer::OnDestroy() {
    transport.Close();
    listening = false;
}

void NetworkServer::Send(ConnectionId client, NetChannel channel, MessageType type, const ByteWriter& payload) {
    const std::vector<std::uint8_t> bytes = FrameMessage(type, payload);
    transport.Send(client, channel, bytes.data(), bytes.size());
}

void NetworkServer::Broadcast(NetChannel channel, MessageType type, const ByteWriter& payload) {
    const std::vector<std::uint8_t> bytes = FrameMessage(type, payload);
    transport.Broadcast(channel, bytes.data(), bytes.size());
}

void NetworkServer::Update(float /*deltaTime*/) {
    transport.Poll(events);

    for (NetEvent& event : events) {
        switch (event.type) {
            case NetEvent::Type::Connected: {
                ByteWriter welcome;
                welcome.Write(event.connection);
                Send(event.connection, NetChannel::Reliable, NetMessage::Welcome, welcome);
                TraceLog(LOG_INFO, "NET: client %u connected", event.connection);
                if (onClientConnected) {
                    onClientConnected(event.connection);
                }
                break;
            }
            case NetEvent::Type::Disconnected: {
                TraceLog(LOG_INFO, "NET: client %u disconnected", event.connection);
                if (onClientDisconnected) {
                    onClientDisconnected(event.connection);
                }
                break;
            }
            case NetEvent::Type::Received: {
                ByteReader reader(event.payload.data(), event.payload.size());
                MessageType type = 0;
                if (!reader.Read(type)) {
                    break;
                }
                auto handler = handlers.find(type);
                if (handler != handlers.end()) {
                    handler->second(event.connection, reader);
                }
                break;
            }
        }
    }
}

void NetworkServer::LateUpdate(float deltaTime) {
    serverTime += deltaTime;
    snapshotTimer += deltaTime;

    const float interval = 1.0f / SnapshotRate;
    if (snapshotTimer >= interval) {
        snapshotTimer = snapshotTimer >= 2.0f * interval ? 0.0f : snapshotTimer - interval;
        SendSnapshot();
    }

    transport.Flush();
}

void NetworkServer::SendSnapshot() {
    if (ClientCount() == 0) {
        return;
    }

    std::vector<const NetworkIdentity*> replicated;
    for (const auto& [netId, identity] : NetworkIdentity::Registry()) {
        if (identity->GetTransform() != nullptr && identity->owner->IsActiveInHierarchy()) {
            replicated.push_back(identity);
        }
    }

    ByteWriter snapshot;
    snapshot.Write(++snapshotTick);
    snapshot.Write(serverTime);
    snapshot.Write(static_cast<std::uint16_t>(replicated.size()));
    for (const NetworkIdentity* identity : replicated) {
        snapshot.Write(identity->GetNetId());
        snapshot.Write(identity->GetTransform()->GetWorldPosition());
        snapshot.Write(identity->GetTransform()->GetWorldRotation());
    }

    Broadcast(NetChannel::Unreliable, NetMessage::Snapshot, snapshot);
}

}
