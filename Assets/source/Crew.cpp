#include "Crew.h"
#include "Scrapper/Core/Parts.h"
#include "Scrapper/Jolt/SpringCharacter.h"
#include "Scrapper/Net/NetId.h"
#include "Scrapper/Net/NetOwner.h"
#include "Scrapper/Raylib/Renderers.h"

using namespace Scrapper;

namespace {
constexpr const char* HeadMesh = "HeadMesh";
}


GameObject SpawnCrewMember(World& world, const std::shared_ptr<const RagdollAsset>& body, std::uint32_t netId, Vector3 feet, Color color) {
    constexpr Color Skin = { 240, 205, 170, 255 };

    GameObject crew = GameObject::Create(world, feet);
    crew.Add<NetId>(netId);
    crew.Add<SpringCharacter>();
    Ragdolls::Add(world.Registry(), crew.Entity(), body);

    const std::vector<entt::entity>& parts = crew.Get<Parts>().entities;
    for (std::size_t i = 0; i < parts.size(); ++i) {
        const RagdollAsset::Part& rest = body->parts[i];
        const bool isHead = rest.name == HeadJoint;
        GameObject mesh = GameObject::Create(world, { 0.0f, 0.0f, 0.0f }, GameObject(world, parts[i]));
        mesh.Add<ModelRenderer>(ModelRenderer{ rest.model, ColliderDisplay::Hidden, {}, isHead ? Skin : color });
        if (!isHead) continue;
        mesh.SetName(HeadMesh);

        // A visor, so others can tell where it looks: forward in character space, turned into the head's frame.
        const Quaternion toHead = QuaternionInvert(rest.rotation);
        GameObject visor = GameObject::Create(world, Vector3RotateByQuaternion({ 0.0f, 0.06f, -0.11f }, toHead), mesh);
        visor.Get<LocalTransform>().rotation = toHead;
        visor.Add<MeshRenderer>(PrimitiveMesh::Cube, Vector3{ 0.16f, 0.05f, 0.05f }, DARKGRAY);
    }
    return crew;
}

void SetHeadVisible(GameObject crew, bool visible) {
    for (entt::entity part : crew.Get<Parts>().entities) {
        if (GameObject mesh = GameObject(crew.GetWorld(), part).FindChild(HeadMesh)) mesh.SetActive(visible);
    }
}

void AssignCrew(const CrewList& crew, ConnectionId client) {
    for (std::size_t slot = 1; slot < crew.size(); ++slot) {
        GameObject member = crew[slot];
        if (!member.Has<NetOwner>()) {
            member.Add<NetOwner>(client);
            return;
        }
    }
}

GameObject FindLocalCrew(const CrewList& crew, const NetSession& session) {
    for (const GameObject& member : crew) {
        if (session.Owns(OwnerOf(member.GetWorld().Registry(), member.Entity()))) return member;
    }
    return {};
}
