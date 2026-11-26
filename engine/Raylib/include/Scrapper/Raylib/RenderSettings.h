#pragma once

#include "Scrapper/Core/Math.h"


namespace Scrapper {
// World resource for scene-wide drawing settings.
struct RenderSettings {
    Color clearColor = BLACK;
    Vector3 lightDirection = { -0.35f, -0.85f, -0.4f }; // direction the light travels
    Vector2 targetResolution = { 0.0f, 0.0f };          // written by the module
};

}
