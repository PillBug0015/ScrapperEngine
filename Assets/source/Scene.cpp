#include "Scene.h"
#include "CrewBody.h"
#include "Tank.h"
#include "Scrapper/Jolt/JoltConversion.h"
#include "Scrapper/Jolt/PhysicsBody.h"
#include "Scrapper/Net/NetId.h"
#include "Scrapper/Raylib/Renderers.h"

#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>

using namespace Scrapper;

namespace {
GameObject SpawnBox(World& world, Vector3 position, Vector3 size, Color color, JPH::EMotionType motion) {
    GameObject box = GameObject::Create(world, position);
    box.Add<MeshRenderer>(PrimitiveMesh::Cube, size, color);
    box.Add<RigidBody>(RigidBody::Make(new JPH::BoxShape(ToJolt(Vector3Scale(size, 0.5f))), motion));
    return box;
}
}


std::optional<Scene> Scene::Build(World& world) {
    Scene out;
    SpawnBox(world, { 0.0f, -0.5f, 0.0f }, { 30.0f, 1.0f, 30.0f }, { 95, 140, 85, 255 }, JPH::EMotionType::Static);

    for (int i = 0; i < 12; ++i) {
        const Vector3 position = { (i % 3) * 1.2f - 1.2f, 1.0f + i * 1.1f, (i % 2) * 0.5f - 2.5f };
        SpawnBox(world, position, { 1.0f, 1.0f, 1.0f }, { 210, 150, 80, 255 }, JPH::EMotionType::Dynamic)
            .Add<NetId>(static_cast<std::uint32_t>(100 + i));
    }

    GameObject ball = GameObject::Create(world, { -3.0f, 6.0f, 1.0f });
    ball.Add<MeshRenderer>(PrimitiveMesh::Sphere, Vector3{ 1.2f, 1.2f, 1.2f }, Color{ 200, 70, 70, 255 });
    ball.Add<RigidBody>(RigidBody::Make(new JPH::SphereShape(0.6f), JPH::EMotionType::Dynamic));
    ball.Add<NetId>(2u);

    // Greybox tank from Assets/Models/Tank.blend (exported to Tank.glb by the build).
    const std::shared_ptr<const ModelAsset> model = LoadModelAsset("Models/Tank.glb");
    if (model == nullptr) return std::nullopt;
    const Vector3 tankPosition = { 5.0f, 0.05f, 2.0f };
    out.tank = SpawnTank(world, model, tankPosition);
    out.tank.Add<NetId>(1u);

    const std::optional<CrewBody> body = MakeCrewBody();
    if (!body) return std::nullopt;

    // Three crew start at the tank's stations, the fourth outside by the step.
    const auto station = [&](const char* name) {
        const ModelMarker* marker = model->FindMarker(name);
        return Vector3Add(tankPosition, marker != nullptr ? marker->position : Vector3{ 0.0f, 1.0f, 0.0f });
    };
    out.crew = {
        SpawnCrewMember(world, *body, 10, station("STATION_Driver"), Color{ 200, 60, 60, 255 }),
        SpawnCrewMember(world, *body, 11, station("STATION_Gunner"), Color{ 60, 110, 210, 255 }),
        SpawnCrewMember(world, *body, 12, station("STATION_Engine"), Color{ 230, 190, 50, 255 }),
        SpawnCrewMember(world, *body, 13, Vector3Add(tankPosition, { -3.2f, 0.0f, 0.0f }), Color{ 70, 170, 90, 255 }),
    };
    return out;
}
