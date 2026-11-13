#pragma once

#include "Core/Component/Component.h"
#include "Network/ByteStream.h"
#include "Network/NetTransport.h"

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>


namespace ScrapperEngine {
// Connects to a NetworkServer and shows every NetworkIdentity slightly in the past,
// interpolating between buffered snapshots so 30 Hz updates render smoothly.
class NetworkClient : public Component {
public:
    using MessageHandler = std::function<void(ByteReader&)>;

    // ~2 snapshot intervals: one late packet still leaves a state to interpolate towards.
    static constexpr double InterpolationDelay = 0.07;

    explicit NetworkClient(std::string address, std::uint16_t port = DefaultPort)
        : address(std::move(address)), port(port) {}

    std::function<void()> onConnected;
    std::function<void()> onDisconnected;

    void On(MessageType type, MessageHandler handler) { handlers[type] = std::move(handler); }
    void Send(NetChannel channel, MessageType type, const ByteWriter& payload = {});

    bool IsConnected() const { return server != 0; }
    ConnectionId GetClientId() const { return clientId; }
    const std::string& GetAddress() const { return address; }

    void Start() override;
    void OnDestroy() override;
    void Update(float deltaTime) override;
    void LateUpdate(float deltaTime) override;

private:
    NetTransport transport;
    std::string address;
    std::uint16_t port;

    ConnectionId server = 0;   // our connection to the server, 0 while disconnected
    ConnectionId clientId = 0; // how the server knows us (from Welcome)

    std::unordered_map<MessageType, MessageHandler> handlers;
    std::vector<NetEvent> events;

    std::uint32_t lastSnapshotTick = 0;
    bool hasClockOffset = false;
    double clockOffset = 0.0; // server time - local time

    void ReceiveSnapshot(ByteReader& reader);
};

}
