#include "Network/NetworkClient.h"
#include "Network/NetworkIdentity.h"

#include <raylib.h>


namespace ScrapperEngine {
void NetworkClient::Start() {
    if (transport.Connect(address, port)) {
        TraceLog(LOG_INFO, "NET: connecting to %s:%u", address.c_str(), port);
    } else {
        TraceLog(LOG_WARNING, "NET: invalid server address %s", address.c_str());
    }
}

void NetworkClient::OnDestroy() {
    transport.Close();
    server = 0;
}

void NetworkClient::Send(NetChannel channel, MessageType type, const ByteWriter& payload) {
    if (server == 0) {
        return;
    }
    const std::vector<std::uint8_t> bytes = FrameMessage(type, payload);
    transport.Send(server, channel, bytes.data(), bytes.size());
}

void NetworkClient::Update(float /*deltaTime*/) {
    transport.Poll(events);

    for (NetEvent& event : events) {
        switch (event.type) {
            case NetEvent::Type::Connected: {
                server = event.connection;
                TraceLog(LOG_INFO, "NET: connected to %s", address.c_str());
                if (onConnected) {
                    onConnected();
                }
                break;
            }
            case NetEvent::Type::Disconnected: {
                const bool wasConnected = server != 0;
                server = 0;
                clientId = 0;
                lastSnapshotTick = 0;
                hasClockOffset = false;
                if (wasConnected) {
                    TraceLog(LOG_INFO, "NET: disconnected from %s", address.c_str());
                } else {
                    TraceLog(LOG_WARNING, "NET: could not reach %s", address.c_str());
                }
                if (wasConnected && onDisconnected) {
                    onDisconnected();
                }
                break;
            }
            case NetEvent::Type::Received: {
                ByteReader reader(event.payload.data(), event.payload.size());
                MessageType type = 0;
                if (!reader.Read(type)) {
                    break;
                }

                if (type == NetMessage::Welcome) {
                    reader.Read(clientId);
                } else if (type == NetMessage::Snapshot) {
                    ReceiveSnapshot(reader);
                } else if (auto handler = handlers.find(type); handler != handlers.end()) {
                    handler->second(reader);
                }
                break;
            }
        }
    }

    // Replica poses are set here, before physics, so kinematic bodies follow them this frame.
    const double renderTime = GetTime() + clockOffset - InterpolationDelay;
    for (const auto& [netId, identity] : NetworkIdentity::Registry()) {
        identity->UpdateReplica(renderTime);
    }
}

void NetworkClient::LateUpdate(float /*deltaTime*/) {
    transport.Flush();
}

void NetworkClient::ReceiveSnapshot(ByteReader& reader) {
    std::uint32_t tick = 0;
    double serverTime = 0.0;
    std::uint16_t count = 0;
    if (!reader.Read(tick) || !reader.Read(serverTime) || !reader.Read(count)) {
        return;
    }

    if (tick <= lastSnapshotTick) {
        return; // unreliable channel: late or duplicated packet
    }
    lastSnapshotTick = tick;

    // Smoothed so one delayed packet doesn't jerk the whole timeline.
    const double offsetSample = serverTime - GetTime();
    clockOffset = hasClockOffset ? clockOffset + (offsetSample - clockOffset) * 0.05 : offsetSample;
    hasClockOffset = true;

    for (std::uint16_t i = 0; i < count; ++i) {
        NetId netId = 0;
        NetworkIdentity::State state;
        state.time = serverTime;
        if (!reader.Read(netId) || !reader.Read(state.position) || !reader.Read(state.rotation)) {
            return;
        }

        if (NetworkIdentity* identity = NetworkIdentity::Find(netId)) {
            identity->ReceiveState(state);
        }
    }
}

}
