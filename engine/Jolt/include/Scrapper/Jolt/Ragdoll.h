#pragma once

#include "Scrapper/Assets/Model.h"
#include "Scrapper/Core/Math.h"
#include "Scrapper/Core/TypedIndex.h"
#include "Scrapper/Jolt/JoltRuntime.h"
#include "Scrapper/Jolt/RagdollClip.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Ragdoll/Ragdoll.h>

#include <entt/entity/registry.hpp>

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>


namespace Scrapper {
using PartId = TypedIndex<struct RagdollPartTag>;
using ClipId = TypedIndex<struct RagdollClipTag>;

// One body of a ragdoll: a capsule from `from` to `to`, hinged to its parent at `from`.
// Character space: feet near y = 0, facing -Z, every part at rest unrotated.
struct RagdollPartDesc {
    std::string name;
    std::string parent; // empty for the root, which comes first; parents come before their children
    Vector3 from = { 0.0f, 0.0f, 0.0f };
    Vector3 to = { 0.0f, 0.0f, 0.0f };
    float radius = 0.1f;
    float mass = 1.0f;
    float swingDegrees = 45.0f; // how far the joint bends off rest
    float twistDegrees = 20.0f; // how far it turns around the part
};

struct RagdollDesc {
    std::vector<RagdollPartDesc> parts;
    float motorTorque = 300.0f;   // Nm per joint at full stability; weak motors get stuck on things
    float motorFrequency = 10.0f; // Hz: how stiffly the joints chase their clip pose
};

// Built once, shared by every ragdoll of this kind. Names are resolved into ids when it is built;
// after that everything goes by id.
struct RagdollAsset {
    struct Part {
        std::string name;
        std::optional<PartId> parent; // none for the root
        Vector3 position = { 0.0f, 0.0f, 0.0f }; // rest pose, character space, lowest point on y = 0
        Quaternion rotation = { 0.0f, 0.0f, 0.0f, 1.0f };
        std::shared_ptr<const ModelAsset> model; // the part's collision shape as a mesh, in part space
    };
    struct Clip {
        std::string name;
        float duration = 1.0f;
        bool loop = true;
        std::vector<std::vector<RagdollClip::Key>> keysByPart;

        Quaternion Sample(PartId part, float time) const; // relative to the parent; rest when the part has no keys
    };
    static constexpr PartId Root{ 0 };

    JoltRuntime runtime; // first: the Jolt objects below need it until the end
    JPH::Ref<JPH::RagdollSettings> settings;
    std::vector<Part> parts; // in PartId order, Root first
    std::vector<Clip> clips; // in ClipId order
    float motorTorque = 0.0f;

    const Part& At(PartId id) const { return parts[id.value]; }
    const Clip& At(ClipId id) const { return clips[id.value]; }
    std::optional<PartId> FindPart(std::string_view name) const;
    std::optional<ClipId> FindClip(std::string_view name) const;
};

// nullptr (and a log line) when the description doesn't hold together, e.g. a clip names a part it doesn't have.
std::shared_ptr<const RagdollAsset> MakeRagdollAsset(const RagdollDesc& desc, const std::vector<RagdollClip>& clips);

// Authored: the entity's body is this ragdoll. Give it one through Ragdolls::Add.
struct Ragdoll {
    std::shared_ptr<const RagdollAsset> asset;
};

namespace Ragdolls {
// The parts become the entity's children right away (Parts, named after them, posed at rest
// relative to the entity), ready to decorate; the Jolt module gives them bodies at its next step.
// From then on the entity itself follows the root part.
void Add(entt::registry& registry, entt::entity entity, std::shared_ptr<const RagdollAsset> asset);
// The child entity that is this part of the entity's ragdoll.
entt::entity PartEntity(const entt::registry& registry, entt::entity entity, PartId part);
}

}
