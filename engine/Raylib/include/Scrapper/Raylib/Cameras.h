#pragma once

#include "Scrapper/Core/Math.h"


namespace Scrapper {
// Looks along the entity's forward (-Z). Cameras draw in ascending `order`.
struct CameraView3D {
    float fovy = 60.0f;
    int projection = CAMERA_PERSPECTIVE;
    int order = 0;
};

struct CameraView2D {
    Vector2 offset = { 0.0f, 0.0f };
    float zoom = 1.0f;
    int order = 0;
};

}
