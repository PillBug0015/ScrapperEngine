#pragma once

#include "Scrapper/Net/ByteStream.h"
#include "Scrapper/Net/NetTypes.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>


namespace Scrapper {
class NetTransport;

enum class NetRole { Server, Client };

// World resource for game messages. Snapshots of NetId entities are handled by the module itself.
class NetSession {
public:
    using MessageHandler = std::function<void(ConnectionId sender, ByteReader&)>; // on clients the sender is the server
    using ConnectionHandler = std::function<void(ConnectionId)>;

    NetSession(NetRole role, std::string address, std::uint16_t port);
    ~NetSession();
    NetSession(const NetSession&) = delete;
    NetSession& operator=(const NetSession&) = delete;

    NetRole Role() const { return role; }
    bool IsServer() const { return role == NetRole::Server; }
    const std::string& Address() const { return address; }

    void On(MessageType type, MessageHandler handler) { handlers[type] = std::move(handler); }
    ConnectionHandler onConnected;    // server: a client joined.  client: we reached the server
    ConnectionHandler onDisconnected;

    // Server side
    void Send(ConnectionId to, NetChannel channel, MessageType type, const ByteWriter& payload = {});
    void Broadcast(NetChannel channel, MessageType type, const ByteWriter& payload = {});
    std::size_t ClientCount() const;

    // Client side
    void SendToServer(NetChannel channel, MessageType type, const ByteWriter& payload = {});
    bool IsConnected() const { return server != 0; }

    // How the server knows this peer: ServerOwner on the server, the Welcome's id on a client.
    ConnectionId LocalId() const { return localId; }
    // Whether this peer simulates what `owner` owns. A client owns nothing until welcomed.
    bool Owns(ConnectionId owner) const { return IsServer() ? owner == ServerOwner : localId != 0 && owner == localId; }

private:
    friend struct NetSystems;

    NetRole role;
    std::string address;
    std::uint16_t port;
    std::unique_ptr<NetTransport> transport;
    std::unordered_map<MessageType, MessageHandler> handlers;

    ConnectionId server = 0; // client: our connection to the server
    ConnectionId localId = ServerOwner;
};

}
