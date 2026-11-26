#include "Tank.h"
#include "Scrapper/Jolt/PhysicsBody.h"
#include "Scrapper/Raylib/Renderers.h"
#include "Scrapper/ScriptEngine/ForJolt.h"

using namespace Scrapper;

namespace {
constexpr float TankMass = 6000.0f;
constexpr float TopSpeed = 5.0f;    // m/s
constexpr float TopTurnRate = 1.0f; // rad/s
}


GameObject SpawnTank(World& world, const std::shared_ptr<const ModelAsset>& model, Vector3 position) {
    GameObject tank = GameObject::Create(world, position);
    tank.SetName("Tank");
    tank.Add<ModelRenderer>(ModelRenderer{ model, ColliderDisplay::Solid });
    tank.Add<RigidBody>(RigidBody::FromConvexParts(model->colliders, JPH::EMotionType::Dynamic, TankMass));
    for (const ModelMarker& marker : model->markers) {
        GameObject spot = GameObject::Create(world, marker.position, tank);
        spot.Get<LocalTransform>().rotation = marker.rotation;
        spot.SetName(marker.name);
    }
    return tank;
}

void TankDrive::SetInput(ConnectionId from, Vector2 input) {
    inputs[from] = { Clamp(input.x, -1.0f, 1.0f), Clamp(input.y, -1.0f, 1.0f) }; // never trust a client's magnitude
}

void TankDrive::Forget(ConnectionId from) { inputs.erase(from); }

void TankDrive::Shove() { ForJolt::Body(gameObject).SetLinearVelocity({ 1.5f, 7.0f, 0.0f }); }

void TankDrive::Update(float) {
    Vector2 total = { 0.0f, 0.0f };
    for (const auto& [from, input] : inputs) total = Vector2Add(total, input);
    total = { Clamp(total.x, -1.0f, 1.0f), Clamp(total.y, -1.0f, 1.0f) };

    ForJolt::Body body(gameObject);
    Vector3 forward = gameObject.Forward();
    forward.y = 0.0f;
    forward = Vector3Normalize(forward);
    const float speed = Vector3DotProduct(body.GetLinearVelocity(), forward);
    if ((total.y > 0.0f && speed < TopSpeed) || (total.y < 0.0f && speed > -TopSpeed)) {
        body.AddForce(Vector3Scale(forward, total.y * TankMass * 6.0f));
    }
    const float turnRate = body.GetAngularVelocity().y;
    if ((total.x < 0.0f && turnRate < TopTurnRate) || (total.x > 0.0f && turnRate > -TopTurnRate)) {
        body.AddTorque({ 0.0f, -total.x * TankMass * 8.0f, 0.0f });
    }
}
