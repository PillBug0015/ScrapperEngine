#pragma once

#include "RectRenderer.h"

class SpriteComponent : public RectRenderer {
public:
    SpriteComponent(Vector2 size, Color color, int layer = 0)
        : RectRenderer(size, color, layer) {}
};
