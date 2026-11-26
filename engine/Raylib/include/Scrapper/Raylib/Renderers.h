#pragma once

#include "Scrapper/Assets/Model.h"
#include "Scrapper/Core/Math.h"

#include <memory>
#include <string>


namespace Scrapper {
enum class PrimitiveMesh {
    Cube,     // 1 x 1 x 1, centered
    Sphere,   // diameter 1
    Cylinder, // diameter 1, height 1 along Y, centered
    Plane,    // 1 x 1 on XZ, facing +Y
    Count
};

// Unit primitive on the entity's WorldTransform, stretched to `size`.
struct MeshRenderer {
    PrimitiveMesh mesh = PrimitiveMesh::Cube;
    Vector3 size = { 1.0f, 1.0f, 1.0f };
    Color color = WHITE;
};

enum class ColliderDisplay { Hidden, Solid, Wireframe };

// A loaded model on the entity's WorldTransform. Showing the colliders helps check how they fit,
// and is how a greybox model (colliders only) is seen at all.
struct ModelRenderer {
    std::shared_ptr<const ModelAsset> model;
    ColliderDisplay colliders = ColliderDisplay::Hidden;
    Color colliderColor = { 120, 150, 110, 255 };
    Color tint = WHITE; // multiplies the model's own colors
};

// 2D renderers draw in the 2D world, or on the UI when the entity also has a RectLayout.
// Position is the WorldTransform's x/y, rotation its rotation around Z.
struct RectRenderer {
    Vector2 size = { 100.0f, 100.0f };
    Color color = WHITE;
    Vector2 pivot = { 0.5f, 0.5f };
    int layer = 0;
};

struct TextRenderer {
    std::string text;
    int fontSize = 20;
    Color color = WHITE;
    int layer = 0;
};

struct SpriteRenderer {
    Texture2D texture = {};
    Rectangle source = { 0.0f, 0.0f, 0.0f, 0.0f };
    Vector2 size = { 0.0f, 0.0f };
    Color tint = WHITE;
    Vector2 pivot = { 0.5f, 0.5f };
    int layer = 0;
};

}
