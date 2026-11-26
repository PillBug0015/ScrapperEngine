#include "Scrapper/Net/NetSession.h"
#include "NetTransport.h"


namespace Scrapper {
namespace {
// Wire format of every message: [MessageType][payload].
std::vector<std::uint8_t> Frame(MessageType type, const ByteWriter& payload) {
    std::vector<std::uint8_t> bytes(sizeof(MessageType) + payload.Size());
    std::memcpy(bytes.data(), &type, sizeof(MessageType));
    if (payload.Size() > 0) std::memcpy(bytes.data() + sizeof(MessageType), payload.Data(), payload.Size());
    return bytes;
}
}

NetSession::NetSession(NetRole role, std::string address, std::uint16_t port)
    : role(role), address(std::move(address)), port(port), transport(std::make_unique<NetTransport>()) {}

NetSession::~NetSession() = default;

void NetSession::Send(ConnectionId to, NetChannel channel, MessageType type, const ByteWriter& payload) {
    const std::vector<std::uint8_t> bytes = Frame(type, payload);
    transport->Send(to, channel, bytes.data(), bytes.size());
}

void NetSession::Broadcast(NetChannel channel, MessageType type, const ByteWriter& payload) {
    const std::vector<std::uint8_t> bytes = Frame(type, payload);
    transport->Broadcast(channel, bytes.data(), bytes.size());
}

std::size_t NetSession::ClientCount() const {
    return IsServer() ? transport->ConnectionCount() : 0;
}

void NetSession::SendToServer(NetChannel channel, MessageType type, const ByteWriter& payload) {
    if (server != 0) Send(server, channel, type, payload);
}

}
