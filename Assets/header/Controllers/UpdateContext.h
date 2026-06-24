#pragma once

#include "Input/InputState.h"

namespace mud {
    struct UpdateContext
    {
        float DeltaTime = 0.0f;
        InputState Input;
    };
}
