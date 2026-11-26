#include "ShapeModel.h"

#include <Jolt/Geometry/AABox.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/ShapeFilter.h>
#include <Jolt/Physics/Collision/TransformedShape.h>

#include <array>


namespace Scrapper {
std::shared_ptr<const ModelAsset> MakeShapeModel(const JPH::Shape& shape, const std::string& name) {
    MeshPart mesh;
    mesh.name = name;

    // Only leaf shapes give triangles: unpack compounds and offsets first.
    JPH::AllHitCollisionCollector<JPH::TransformedShapeCollector> leaves;
    shape.CollectTransformedShapes(JPH::AABox::sBiggest(), shape.GetCenterOfMass(), JPH::Quat::sIdentity(), JPH::Vec3::sOne(),
                                   JPH::SubShapeIDCreator(), leaves, JPH::ShapeFilter());

    std::array<JPH::Float3, 3 * 64> vertices;
    for (const JPH::TransformedShape& leaf : leaves.mHits) {
        JPH::Shape::GetTrianglesContext context;
        leaf.GetTrianglesStart(context, JPH::AABox::sBiggest(), JPH::RVec3::sZero());
        for (int count; (count = leaf.GetTrianglesNext(context, 64, vertices.data())) > 0;) {
            for (int t = 0; t < count; ++t) {
                const Vector3 a = { vertices[3 * t].x, vertices[3 * t].y, vertices[3 * t].z };
                const Vector3 b = { vertices[3 * t + 1].x, vertices[3 * t + 1].y, vertices[3 * t + 1].z };
                const Vector3 c = { vertices[3 * t + 2].x, vertices[3 * t + 2].y, vertices[3 * t + 2].z };
                const Vector3 normal = Vector3Normalize(Vector3CrossProduct(Vector3Subtract(b, a), Vector3Subtract(c, a)));
                for (const Vector3& v : { a, b, c }) {
                    mesh.indices.push_back(static_cast<std::uint32_t>(mesh.positions.size()));
                    mesh.positions.push_back(v);
                    mesh.normals.push_back(normal);
                }
            }
        }
    }

    auto model = std::make_shared<ModelAsset>();
    model->path = name;
    model->meshes.push_back(std::move(mesh));
    return model;
}

}
