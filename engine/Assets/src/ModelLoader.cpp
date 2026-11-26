#include "Scrapper/Assets/Model.h"

// Declarations only: the implementation is compiled into raylib (see CMakeLists.txt).
#include <cgltf.h>

#include <cmath>
#include <cstdio>
#include <cstring>


namespace Scrapper {
namespace {
bool StartsWith(const char* text, const char* prefix) {
    return text != nullptr && std::strncmp(text, prefix, std::strlen(prefix)) == 0;
}

// cgltf gives column-major float[16]; raylib's Matrix names its fields m0..m15 in the same order.
Matrix WorldMatrix(const cgltf_node* node) {
    float m[16];
    cgltf_node_transform_world(node, m);
    Matrix out;
    out.m0 = m[0];  out.m1 = m[1];  out.m2 = m[2];   out.m3 = m[3];
    out.m4 = m[4];  out.m5 = m[5];  out.m6 = m[6];   out.m7 = m[7];
    out.m8 = m[8];  out.m9 = m[9];  out.m10 = m[10]; out.m11 = m[11];
    out.m12 = m[12]; out.m13 = m[13]; out.m14 = m[14]; out.m15 = m[15];
    return out;
}

// glTF stores linear color; raylib draws in sRGB, which is what Blender showed.
unsigned char ToSrgb(float linear) {
    const float srgb = std::pow(Clamp(linear, 0.0f, 1.0f), 1.0f / 2.2f);
    return static_cast<unsigned char>(srgb * 255.0f + 0.5f);
}

const cgltf_accessor* FindAttribute(const cgltf_primitive& primitive, cgltf_attribute_type type) {
    for (cgltf_size i = 0; i < primitive.attributes_count; ++i) {
        if (primitive.attributes[i].type == type) return primitive.attributes[i].data;
    }
    return nullptr;
}

// Appends one triangle primitive, transformed into model space.
void AppendPrimitive(const cgltf_primitive& primitive, const Matrix& world, std::vector<Vector3>& positions,
                     std::vector<Vector3>* normals, std::vector<std::uint32_t>& indices) {
    const cgltf_accessor* position = FindAttribute(primitive, cgltf_attribute_type_position);
    if (primitive.type != cgltf_primitive_type_triangles || position == nullptr) return;

    const std::uint32_t base = static_cast<std::uint32_t>(positions.size());
    for (cgltf_size i = 0; i < position->count; ++i) {
        float p[3] = {};
        cgltf_accessor_read_float(position, i, p, 3);
        positions.push_back(Vector3Transform({ p[0], p[1], p[2] }, world));
    }

    const cgltf_accessor* normal = FindAttribute(primitive, cgltf_attribute_type_normal);
    if (normals != nullptr && normal != nullptr) {
        const Matrix normalMatrix = MatrixTranspose(MatrixInvert(world));
        for (cgltf_size i = 0; i < normal->count; ++i) {
            float n[3] = {};
            cgltf_accessor_read_float(normal, i, n, 3);
            const Vector3 v = { n[0], n[1], n[2] };
            normals->push_back(Vector3Normalize({
                normalMatrix.m0 * v.x + normalMatrix.m4 * v.y + normalMatrix.m8 * v.z,
                normalMatrix.m1 * v.x + normalMatrix.m5 * v.y + normalMatrix.m9 * v.z,
                normalMatrix.m2 * v.x + normalMatrix.m6 * v.y + normalMatrix.m10 * v.z }));
        }
    }

    if (primitive.indices != nullptr) {
        for (cgltf_size i = 0; i < primitive.indices->count; ++i) {
            indices.push_back(base + static_cast<std::uint32_t>(cgltf_accessor_read_index(primitive.indices, i)));
        }
    } else {
        for (cgltf_size i = 0; i < position->count; ++i) indices.push_back(base + static_cast<std::uint32_t>(i));
    }
}
}

const ModelMarker* ModelAsset::FindMarker(const std::string& name) const {
    for (const ModelMarker& marker : markers) {
        if (marker.name == name) return &marker;
    }
    return nullptr;
}

std::shared_ptr<const ModelAsset> LoadModelAsset(const std::string& path) {
    cgltf_options options = {};
    cgltf_data* data = nullptr;
    if (cgltf_parse_file(&options, path.c_str(), &data) != cgltf_result_success) {
        std::fprintf(stderr, "[Assets] can't read %s\n", path.c_str());
        return nullptr;
    }
    if (cgltf_load_buffers(&options, data, path.c_str()) != cgltf_result_success) {
        std::fprintf(stderr, "[Assets] can't load the buffers of %s\n", path.c_str());
        cgltf_free(data);
        return nullptr;
    }

    auto asset = std::make_shared<ModelAsset>();
    asset->path = path;

    for (cgltf_size n = 0; n < data->nodes_count; ++n) {
        const cgltf_node& node = data->nodes[n];
        const char* name = node.name != nullptr ? node.name : "";
        const Matrix world = WorldMatrix(&node);

        if (node.mesh == nullptr) {
            if (node.children_count == 0) {
                asset->markers.push_back({ name, { world.m12, world.m13, world.m14 }, QuaternionNormalize(QuaternionFromMatrix(world)) });
            }
            continue;
        }

        if (StartsWith(name, "COL_")) {
            ConvexPart part;
            part.name = name;
            for (cgltf_size p = 0; p < node.mesh->primitives_count; ++p) {
                AppendPrimitive(node.mesh->primitives[p], world, part.positions, nullptr, part.indices);
            }
            asset->colliders.push_back(std::move(part));
            continue;
        }

        // One part per primitive: each primitive has its own material.
        for (cgltf_size p = 0; p < node.mesh->primitives_count; ++p) {
            const cgltf_primitive& primitive = node.mesh->primitives[p];
            MeshPart part;
            part.name = name;
            if (primitive.material != nullptr && primitive.material->has_pbr_metallic_roughness) {
                const cgltf_float* c = primitive.material->pbr_metallic_roughness.base_color_factor;
                part.color = { ToSrgb(c[0]), ToSrgb(c[1]), ToSrgb(c[2]), static_cast<unsigned char>(Clamp(c[3], 0.0f, 1.0f) * 255.0f) };
            }
            AppendPrimitive(primitive, world, part.positions, &part.normals, part.indices);
            asset->meshes.push_back(std::move(part));
        }
    }

    cgltf_free(data);
    std::fprintf(stderr, "[Assets] %s: %zu meshes, %zu colliders, %zu markers\n", path.c_str(),
                 asset->meshes.size(), asset->colliders.size(), asset->markers.size());
    return asset;
}

}
