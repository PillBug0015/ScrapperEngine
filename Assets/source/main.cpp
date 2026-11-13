#include "ScrapperEngine.h"

#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>

#include <array>
#include <cstdint>
#include <string>

using namespace ScrapperEngine;

namespace {
constexpr MessageType PokeMessage = NetMessage::FirstGameMessage;
constexpr MessageType CrewInputMessage = NetMessage::FirstGameMessage + 1;
constexpr MessageType KnockMessage = NetMessage::FirstGameMessage + 2;
constexpr int CrewCount = 4; // slot 0: host, slots 1..3: clients by ConnectionId

GameObject* SpawnBox(Engine& engine, Vector3 position, Vector3 size, Color color, JPH::EMotionType motion) {
    GameObject* box = engine.Instantiate3D(position);
    box->AddComponent<MeshRenderer>(PrimitiveMesh::Cube, size, color);
    box->AddComponent<RigidBody>(new JPH::BoxShape(ToJolt(Vector3Scale(size, 0.5f))), motion);
    return box;
}

CharacterBody* SpawnCrew(Engine& engine, NetId netId, Vector3 feet, Color color) {
    GameObject* crew = engine.Instantiate3D(feet);
    Transform3D* root = crew->GetComponent<Transform3D>();
    engine.Instantiate3D({ 0.0f, 0.65f, 0.0f }, root)->AddComponent<MeshRenderer>(PrimitiveMesh::Cylinder, Vector3{ 0.7f, 1.1f, 0.7f }, color);
    engine.Instantiate3D({ 0.0f, 1.45f, 0.0f }, root)->AddComponent<MeshRenderer>(PrimitiveMesh::Sphere, Vector3{ 0.6f, 0.6f, 0.6f }, Color{ 240, 205, 170, 255 });
    engine.Instantiate3D({ 0.0f, 1.45f, -0.3f }, root)->AddComponent<MeshRenderer>(PrimitiveMesh::Cube, Vector3{ 0.35f, 0.12f, 0.12f }, DARKGRAY); // shows facing
    crew->AddComponent<NetworkIdentity>(netId);
    return crew->AddComponent<CharacterBody>();
}

// Host: drives crew slot 0 from its keyboard and applies each client's input to that client's slot.
// Client: only sends its keyboard input; what it sees comes back through snapshots.
class SandboxController : public Component {
public:
    SandboxController(NetworkServer* server, NetworkClient* client, std::array<CharacterBody*, CrewCount> crew,
                      RigidBody* tank, TextRenderer* status, const Transform3D* camera)
        : server(server), client(client), crew(crew), tank(tank), status(status), camera(camera) {}

    void ApplyCrewInput(ConnectionId slot, Vector3 move, std::uint8_t jumpCount) {
        if (slot >= CrewCount) {
            return;
        }
        if (Vector3LengthSqr(move) > 1.0f) {
            move = Vector3Normalize(move); // never trust a client's input magnitude
        }

        crew[slot]->SetMoveInput(move);
        if (jumpCount != lastJumpCount[slot]) {
            lastJumpCount[slot] = jumpCount;
            crew[slot]->Jump();
        }
    }

    void ReleaseSlot(ConnectionId slot) {
        if (slot < CrewCount) {
            crew[slot]->SetMoveInput({ 0.0f, 0.0f, 0.0f });
        }
    }

    void Poke() {
        tank->SetLinearVelocity({ 1.5f, 7.0f, 0.0f });
    }

    void KnockOver(ConnectionId slot) {
        if (slot >= CrewCount) {
            return;
        }
        // Shoved backwards relative to where the crew member faces, plus a little hop.
        const Vector3 back = Vector3Negate(crew[slot]->owner->GetComponent<Transform3D>()->Forward());
        crew[slot]->Knockback({ back.x * 320.0f, 360.0f, back.z * 320.0f }, 2.0f);
    }

    void Update(float /*deltaTime*/) override {
        const Vector3 move = ReadMoveInput();
        if (IsKeyPressed(KEY_SPACE)) {
            ++jumpCount; // a counter, not a flag: a lost or repeated packet can't lose or double a jump
        }
        const bool poke = IsKeyPressed(KEY_E);
        const bool knock = IsKeyPressed(KEY_Q);

        if (client != nullptr) {
            ByteWriter input;
            input.Write(move);
            input.Write(jumpCount);
            client->Send(NetChannel::Unreliable, CrewInputMessage, input);
            if (poke) {
                client->Send(NetChannel::Reliable, PokeMessage);
            }
            if (knock) {
                client->Send(NetChannel::Reliable, KnockMessage);
            }
        } else {
            ApplyCrewInput(0, move, jumpCount);
            if (poke) {
                Poke();
            }
            if (knock) {
                KnockOver(0);
            }
            // The server simulates every character, so only the host can turn the global knob.
            if (IsKeyPressed(KEY_LEFT_BRACKET)) CharacterBody::globalDamping *= 0.8f;
            if (IsKeyPressed(KEY_RIGHT_BRACKET)) CharacterBody::globalDamping *= 1.25f;
        }

        const char* controls = "WASD move  SPACE jump  Q fall over  E poke tank";
        if (client == nullptr) {
            status->text = TextFormat("HOST  clients: %d   %s   [ ] damping x%.2f", static_cast<int>(server->ClientCount()),
                                      controls, CharacterBody::globalDamping);
        } else if (client->IsConnected()) {
            status->text = TextFormat("CLIENT #%u   %s", client->GetClientId(), controls);
        } else {
            status->text = TextFormat("CLIENT  connecting to %s ...", client->GetAddress().c_str());
        }
    }

private:
    NetworkServer* server;
    NetworkClient* client;
    std::array<CharacterBody*, CrewCount> crew;
    std::array<std::uint8_t, CrewCount> lastJumpCount = {};
    RigidBody* tank;
    TextRenderer* status;
    const Transform3D* camera;
    std::uint8_t jumpCount = 0;

    // WASD relative to the camera, flattened onto the ground.
    Vector3 ReadMoveInput() const {
        Vector3 forward = camera->Forward();
        forward.y = 0.0f;
        forward = Vector3Normalize(forward);
        const Vector3 right = { -forward.z, 0.0f, forward.x };

        Vector3 move = { 0.0f, 0.0f, 0.0f };
        if (IsKeyDown(KEY_W)) move = Vector3Add(move, forward);
        if (IsKeyDown(KEY_S)) move = Vector3Subtract(move, forward);
        if (IsKeyDown(KEY_D)) move = Vector3Add(move, right);
        if (IsKeyDown(KEY_A)) move = Vector3Subtract(move, right);
        return Vector3LengthSqr(move) > 0.0f ? Vector3Normalize(move) : move;
    }
};
}

int main(int argc, char** argv) {
    // No arguments: host a game.  "--join <address>": join one.
    const bool joining = argc >= 3 && std::string(argv[1]) == "--join";

    Engine engine(1280, 720, joining ? "ScrapperEngine - Client" : "ScrapperEngine - Host", 1280, 720);
    if (!engine.Initialize()) {
        return 1;
    }

    RenderSystem::Instance().SetClearColor({ 150, 190, 230, 255 });

    GameObject* camera = engine.Instantiate3D({ 10.0f, 7.0f, 14.0f });
    camera->AddComponent<Camera3DComponent>();
    Transform3D* cameraTransform = camera->GetComponent<Transform3D>();
    cameraTransform->LookAt({ 0.0f, 1.0f, 0.0f });

    // Host and clients build the same scene; the network layer turns client copies into replicas.
    SpawnBox(engine, { 0.0f, -0.5f, 0.0f }, { 30.0f, 1.0f, 30.0f }, { 95, 140, 85, 255 }, JPH::EMotionType::Static);

    for (int i = 0; i < 12; ++i) {
        const Vector3 position = { (i % 3) * 1.2f - 1.2f, 1.0f + i * 1.1f, (i % 2) * 0.5f - 2.5f };
        GameObject* crate = SpawnBox(engine, position, { 1.0f, 1.0f, 1.0f }, { 210, 150, 80, 255 }, JPH::EMotionType::Dynamic);
        crate->AddComponent<NetworkIdentity>(100 + i);
    }

    // The turret and barrel have no bodies: they ride along through the transform hierarchy.
    GameObject* tank = SpawnBox(engine, { 3.0f, 4.0f, 1.5f }, { 2.4f, 0.8f, 1.6f }, { 85, 105, 75, 255 }, JPH::EMotionType::Dynamic);
    tank->AddComponent<NetworkIdentity>(1);
    Transform3D* tankTransform = tank->GetComponent<Transform3D>();
    tankTransform->SetLocalEulerDegrees({ 0.0f, 30.0f, 35.0f });

    GameObject* turret = engine.Instantiate3D({ 0.0f, 0.6f, 0.0f }, tankTransform);
    turret->AddComponent<MeshRenderer>(PrimitiveMesh::Cylinder, Vector3{ 1.0f, 0.45f, 1.0f }, Color{ 70, 90, 60, 255 });

    GameObject* barrel = engine.Instantiate3D({ 1.0f, 0.0f, 0.0f }, turret->GetComponent<Transform3D>());
    barrel->AddComponent<MeshRenderer>(PrimitiveMesh::Cube, Vector3{ 1.6f, 0.18f, 0.18f }, Color{ 60, 75, 50, 255 });

    GameObject* ball = engine.Instantiate3D({ -3.0f, 6.0f, 1.0f });
    ball->AddComponent<MeshRenderer>(PrimitiveMesh::Sphere, Vector3{ 1.2f, 1.2f, 1.2f }, Color{ 200, 70, 70, 255 });
    ball->AddComponent<RigidBody>(new JPH::SphereShape(0.6f), JPH::EMotionType::Dynamic);
    ball->AddComponent<NetworkIdentity>(2);

    const std::array<CharacterBody*, CrewCount> crew = {
        SpawnCrew(engine, 10, { -1.5f, 0.0f, 3.0f }, Color{ 200, 60, 60, 255 }),
        SpawnCrew(engine, 11, { 0.0f, 0.0f, 3.0f }, Color{ 60, 110, 210, 255 }),
        SpawnCrew(engine, 12, { 1.5f, 0.0f, 3.0f }, Color{ 230, 190, 50, 255 }),
        SpawnCrew(engine, 13, { 3.0f, 5.5f, 1.5f }, Color{ 70, 170, 90, 255 }), // drops onto the tank
    };

    GameObject* label = engine.InstantiateRect({ 24.0f, 20.0f }, { 1200.0f, 40.0f });
    TextRenderer* status = label->AddComponent<TextRenderer>("", 24, WHITE, 100);

    GameObject* network = engine.Instantiate();
    NetworkServer* server = nullptr;
    NetworkClient* client = nullptr;
    if (joining) {
        client = network->AddComponent<NetworkClient>(argv[2]);
    } else {
        server = network->AddComponent<NetworkServer>();
    }

    auto* controller = network->AddComponent<SandboxController>(
        server, client, crew, tank->GetComponent<RigidBody>(), status, cameraTransform);

    if (server != nullptr) {
        server->On(PokeMessage, [controller](ConnectionId, ByteReader&) { controller->Poke(); });
        server->On(KnockMessage, [controller](ConnectionId sender, ByteReader&) { controller->KnockOver(sender); });
        server->On(CrewInputMessage, [controller](ConnectionId sender, ByteReader& reader) {
            Vector3 move = { 0.0f, 0.0f, 0.0f };
            std::uint8_t jumpCount = 0;
            if (reader.Read(move) && reader.Read(jumpCount)) {
                controller->ApplyCrewInput(sender, move, jumpCount);
            }
        });
        server->onClientDisconnected = [controller](ConnectionId sender) { controller->ReleaseSlot(sender); };
    }

    engine.Run();
    return 0;
}
