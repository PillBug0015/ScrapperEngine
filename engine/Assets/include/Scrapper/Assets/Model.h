#pragma once

#include "Scrapper/Core/Math.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>


namespace Scrapper {
// Backend-neutral model data read from a .glb. Everything is in model space
// (the glTF scene root): Y up, forward -Z, as tools/blender/export_glb.py writes it.
struct MeshPart {
    std::string name;
    std::vector<Vector3> positions;
    std::vector<Vector3> normals; // may be empty
    std::vector<std::uint32_t> indices;
    Color color = WHITE; // material base color
};

// A COL_ object: one convex piece of the model's collision.
struct ConvexPart {
    std::string name;
    std::vector<Vector3> positions;
    std::vector<std::uint32_t> indices;
};

// An empty without children (STATION_ ...): a named spot for gameplay.
struct ModelMarker {
    std::string name;
    Vector3 position = { 0.0f, 0.0f, 0.0f };
    Quaternion rotation = { 0.0f, 0.0f, 0.0f, 1.0f };
};

struct ModelAsset {
    std::string path;
    std::vector<MeshPart> meshes;
    std::vector<ConvexPart> colliders;
    std::vector<ModelMarker> markers;

    const ModelMarker* FindMarker(const std::string& name) const;
};

// nullptr (and a log line) when the file can't be read.
std::shared_ptr<const ModelAsset> LoadModelAsset(const std::string& path);

}
