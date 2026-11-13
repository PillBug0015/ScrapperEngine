#pragma once

#include "Core/Component/Component.h"
#include "Network/ByteStream.h"
#include "Network/NetTransport.h"

#include <cstdint>
#include <functional>
#include <unordered_map>
#include <vector>


namespace ScrapperEngine {
// Authoritative host. Receives client messages at the start of the frame (Update)
// and broadcasts a snapshot of every NetworkIdentity after physics (LateUpdate).
class NetworkServer : public Component {
public:
    using MessageHandler = std::function<void(ConnectionId, ByteReader&)>;
    using ConnectionHandler = std::function<void(ConnectionId)>;

    static constexpr float SnapshotRate = 30.0f;

    explicit NetworkServer(std::uint16_t port = DefaultPort, std::size_t maxClients = 8)
        : port(port), maxClients(maxClients) {}

    ConnectionHandler onClientConnected;
    ConnectionHandler onClientDisconnected;

    void On(MessageType type, MessageHandler handler) { handlers[type] = std::move(handler); }
    void Send(ConnectionId client, NetChannel channel, MessageType type, const ByteWriter& payload = {});
    void Broadcast(NetChannel channel, MessageType type, const ByteWriter& payload = {});

    bool IsListening() const { return listening; }
    std::size_t ClientCount() const { return transport.ConnectionCount(); }

    void Start() override;
    void OnDestroy() override;
    void Update(float deltaTime) override;
    void LateUpdate(float deltaTime) override;

private:
    NetTransport transport;
    std::uint16_t port;
    std::size_t maxClients;
    bool listening = false;

    std::unordered_map<MessageType, MessageHandler> handlers;
    std::vector<NetEvent> events;

    double serverTime = 0.0;
    float snapshotTimer = 0.0f;
    std::uint32_t snapshotTick = 0;

    void SendSnapshot();
};

}
