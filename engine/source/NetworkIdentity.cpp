#include "Network/NetworkIdentity.h"
#include "Core/GameObject.h"
#include "Core/TransformSystem/Transform3D.h"
#include "Physics/PhysicsBody.h"

#include <raymath.h>


namespace ScrapperEngine {
namespace {
std::unordered_map<NetId, NetworkIdentity*>& MutableRegistry() {
    static std::unordered_map<NetId, NetworkIdentity*> registry;
    return registry;
}
}

void NetworkIdentity::Start() {
    transform = owner->GetComponent<Transform3D>();

    auto [it, inserted] = MutableRegistry().emplace(netId, this);
    if (!inserted) {
        TraceLog(LOG_WARNING, "NetworkIdentity: NetId %u is already in use", netId);
    }
}

void NetworkIdentity::OnDestroy() {
    auto it = MutableRegistry().find(netId);
    if (it != MutableRegistry().end() && it->second == this) {
        MutableRegistry().erase(it);
    }
}

NetworkIdentity* NetworkIdentity::Find(NetId netId) {
    auto it = MutableRegistry().find(netId);
    return it != MutableRegistry().end() ? it->second : nullptr;
}

const std::unordered_map<NetId, NetworkIdentity*>& NetworkIdentity::Registry() {
    return MutableRegistry();
}

void NetworkIdentity::ReceiveState(const State& state) {
    if (historyCount == history.size()) {
        for (std::size_t i = 1; i < history.size(); ++i) {
            history[i - 1] = history[i];
        }
        --historyCount;
    }
    history[historyCount++] = state;
}

void NetworkIdentity::UpdateReplica(double renderTime) {
    // Checked every frame so the order RigidBody and NetworkIdentity were added in doesn't matter.
    if (!bodyIsKinematic) {
        if (PhysicsBody* body = owner->GetComponent<PhysicsBody>()) {
            body->SetMotionType(JPH::EMotionType::Kinematic);
            bodyIsKinematic = true;
        }
    }

    if (transform == nullptr || historyCount == 0) {
        return;
    }

    // Hold the oldest/newest state outside the buffered range; never extrapolate.
    const State* from = &history[0];
    const State* to = &history[0];
    for (std::size_t i = 0; i < historyCount; ++i) {
        if (history[i].time <= renderTime) {
            from = &history[i];
            to = (i + 1 < historyCount) ? &history[i + 1] : &history[i];
        }
    }

    const double span = to->time - from->time;
    const float t = span > 0.0 ? Clamp(static_cast<float>((renderTime - from->time) / span), 0.0f, 1.0f) : 0.0f;

    transform->SetWorldPose(Vector3Lerp(from->position, to->position, t),
                            QuaternionSlerp(from->rotation, to->rotation, t));
}

}
