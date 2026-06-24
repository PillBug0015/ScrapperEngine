#pragma once

namespace mud {
    struct InputState
    {
        bool Up = false;
        bool Down = false;
        bool Left = false;
        bool Right = false;
        bool Confirm = false;
        bool Cancel = false;
        bool Menu = false;
    };
}
