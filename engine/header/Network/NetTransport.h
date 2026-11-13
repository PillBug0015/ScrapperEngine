#pragma once

#include "Network/NetTypes.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>


namespace ScrapperEngine {
struct NetEvent {
    enum class Type { Connected, Disconnected, Received };

    Type type = Type::Received;
    ConnectionId connection = 0;
    NetChannel channel = NetChannel::Reliable;
    std::vector<std::uint8_t> payload;
};

// UDP transport over ENet. ENet stays inside NetTransport.cpp: on Windows it pulls in windows.h,
// whose macros (DrawText, CloseWindow, Rectangle...) break raylib in the same translation unit.
class NetTransport {
public:
    NetTransport();
    ~NetTransport();

    NetTransport(const NetTransport&) = delete;
    NetTransport& operator=(const NetTransport&) = delete;

    bool Listen(std::uint16_t port, std::size_t maxConnections);
    bool Connect(const std::string& address, std::uint16_t port);
    void Close();

    // Receive at the start of a frame, Flush at the end so this frame's sends leave immediately.
    void Poll(std::vector<NetEvent>& outEvents);
    void Flush();
    void Send(ConnectionId connection, NetChannel channel, const std::uint8_t* data, std::size_t size);
    void Broadcast(NetChannel channel, const std::uint8_t* data, std::size_t size);

    std::size_t ConnectionCount() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

}
