#pragma once

#include "Scrapper/Core/Math.h"


namespace Scrapper {
// UI placement in render-target pixels. Raylib.Layout writes the LocalTransform position from it every frame:
// a root is anchored to the render target, a child to its parent's rect.
struct RectLayout {
    Vector2 anchoredPosition = { 0.0f, 0.0f };
    Vector2 anchor = { 0.0f, 0.0f };
    Vector2 pivot = { 0.5f, 0.5f };
    Vector2 size = { 100.0f, 100.0f };
};

}
