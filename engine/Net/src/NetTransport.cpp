// Only translation unit that sees ENet. Do not include raylib (or anything that does) here.
#include "NetTransport.h"

#include <enet/enet.h>

#include <cstdint>
#include <unordered_map>


namespace Scrapper {
namespace {
int libraryUsers = 0;

ConnectionId IdOf(const ENetPeer* peer) {
    return static_cast<ConnectionId>(reinterpret_cast<std::uintptr_t>(peer->data));
}

ENetPacket* MakePacket(NetChannel channel, const std::uint8_t* data, std::size_t size) {
    const enet_uint32 flags = channel == NetChannel::Reliable
        ? ENET_PACKET_FLAG_RELIABLE
        : ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT; // large snapshots stay unreliable when fragmented
    return enet_packet_create(data, size, flags);
}
}

struct NetTransport::Impl {
    ENetHost* host = nullptr;
    std::unordered_map<ConnectionId, ENetPeer*> peers;
    ConnectionId nextId = 1;
};

NetTransport::NetTransport() : impl(std::make_unique<Impl>()) {
    if (libraryUsers++ == 0) {
        enet_initialize();
    }
}

NetTransport::~NetTransport() {
    Close();
    if (--libraryUsers == 0) {
        enet_deinitialize();
    }
}

bool NetTransport::Listen(std::uint16_t port, std::size_t maxConnections) {
    Close();

    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = port;
    impl->host = enet_host_create(&address, maxConnections, NetChannelCount, 0, 0);
    return impl->host != nullptr;
}

bool NetTransport::Connect(const std::string& addressText, std::uint16_t port) {
    Close();

    ENetAddress address;
    if (enet_address_set_host(&address, addressText.c_str()) != 0) {
        return false;
    }
    address.port = port;

    impl->host = enet_host_create(nullptr, 1, NetChannelCount, 0, 0);
    if (impl->host == nullptr) {
        return false;
    }

    if (enet_host_connect(impl->host, &address, NetChannelCount, 0) == nullptr) {
        Close();
        return false;
    }
    return true;
}

void NetTransport::Close() {
    if (impl->host == nullptr) {
        return;
    }

    for (auto& [id, peer] : impl->peers) {
        enet_peer_disconnect_now(peer, 0);
    }
    impl->peers.clear();

    enet_host_destroy(impl->host);
    impl->host = nullptr;
}

void NetTransport::Poll(std::vector<NetEvent>& outEvents) {
    outEvents.clear();
    if (impl->host == nullptr) {
        return;
    }

    ENetEvent event;
    while (enet_host_service(impl->host, &event, 0) > 0) {
        switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT: {
                const ConnectionId id = impl->nextId++;
                event.peer->data = reinterpret_cast<void*>(static_cast<std::uintptr_t>(id));
                impl->peers[id] = event.peer;
                outEvents.push_back({ NetEvent::Type::Connected, id, NetChannel::Reliable, {} });
                break;
            }
            case ENET_EVENT_TYPE_DISCONNECT: {
                // id 0: a connection attempt that never succeeded
                const ConnectionId id = IdOf(event.peer);
                impl->peers.erase(id);
                outEvents.push_back({ NetEvent::Type::Disconnected, id, NetChannel::Reliable, {} });
                break;
            }
            case ENET_EVENT_TYPE_RECEIVE: {
                NetEvent received;
                received.type = NetEvent::Type::Received;
                received.connection = IdOf(event.peer);
                received.channel = static_cast<NetChannel>(event.channelID);
                received.payload.assign(event.packet->data, event.packet->data + event.packet->dataLength);
                outEvents.push_back(std::move(received));
                enet_packet_destroy(event.packet);
                break;
            }
            default:
                break;
        }
    }
}

void NetTransport::Flush() {
    if (impl->host != nullptr) {
        enet_host_flush(impl->host);
    }
}

void NetTransport::Send(ConnectionId connection, NetChannel channel, const std::uint8_t* data, std::size_t size) {
    auto it = impl->peers.find(connection);
    if (it == impl->peers.end()) {
        return;
    }

    ENetPacket* packet = MakePacket(channel, data, size);
    if (enet_peer_send(it->second, static_cast<enet_uint8>(channel), packet) < 0) {
        enet_packet_destroy(packet); // ENet only takes ownership on success
    }
}

void NetTransport::Broadcast(NetChannel channel, const std::uint8_t* data, std::size_t size) {
    if (impl->host == nullptr || impl->peers.empty()) {
        return;
    }

    enet_host_broadcast(impl->host, static_cast<enet_uint8>(channel), MakePacket(channel, data, size));
}

std::size_t NetTransport::ConnectionCount() const {
    return impl->peers.size();
}

}
