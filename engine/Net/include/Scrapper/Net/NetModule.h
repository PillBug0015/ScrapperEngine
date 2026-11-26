#pragma once

#include "Scrapper/Core/Module.h"
#include "Scrapper/Net/NetSession.h"

#include <cstddef>
#include <string>


namespace Scrapper {
namespace NetSystem {
inline constexpr const char* Receive = "Net.Receive";
inline constexpr const char* Authority = "Net.Authority";
inline constexpr const char* Interpolate = "Net.Interpolate";
inline constexpr const char* PlaceRiders = "Net.PlaceRiders";
inline constexpr const char* SendSnapshot = "Net.SendSnapshot";
inline constexpr const char* Flush = "Net.Flush";
}

struct NetConfig {
    NetRole role = NetRole::Server;
    std::string address = "127.0.0.1"; // client: server to join
    std::uint16_t port = DefaultPort;
    std::size_t maxClients = 8;
};

// Every peer simulates the NetId entities it owns (NetOwner) and sends their poses after physics (30 Hz);
// the server relays everyone's. What a peer doesn't own is RemoteAuthority, shown slightly in the past.
class NetModule : public Module {
public:
    explicit NetModule(NetConfig config = {}) : config(std::move(config)) {}

    void Install(World& world) override;
    void Uninstall(World& world) override;

private:
    NetConfig config;
};

}
