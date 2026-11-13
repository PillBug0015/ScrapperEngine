#pragma once

#include "Core/Component/Component.h"
#include "Network/NetTypes.h"

#include <raylib.h>

#include <array>
#include <cstddef>
#include <unordered_map>


namespace ScrapperEngine {
class Transform3D;

// Marks a GameObject (with a Transform3D) as replicated under a game-assigned NetId.
// Server: its pose goes into every snapshot. Client: it is a replica whose pose follows
// interpolated snapshots, and whose PhysicsBody (if any) is switched to kinematic.
class NetworkIdentity : public Component {
public:
    explicit NetworkIdentity(NetId netId) : netId(netId) {}

    NetId GetNetId() const { return netId; }
    Transform3D* GetTransform() const { return transform; }

    void Start() override;
    void OnDestroy() override;

    static NetworkIdentity* Find(NetId netId);
    static const std::unordered_map<NetId, NetworkIdentity*>& Registry();

private:
    friend class NetworkClient;

    struct State {
        double time = 0.0; // server clock
        Vector3 position = { 0.0f, 0.0f, 0.0f };
        Quaternion rotation = { 0.0f, 0.0f, 0.0f, 1.0f };
    };

    void ReceiveState(const State& state);
    void UpdateReplica(double renderTime);

    NetId netId;
    Transform3D* transform = nullptr;
    bool bodyIsKinematic = false;

    std::array<State, 4> history = {}; // ascending by time
    std::size_t historyCount = 0;
};

}
