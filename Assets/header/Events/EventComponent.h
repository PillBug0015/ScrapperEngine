#pragma once

#include <string>

namespace mud {
    class EventComponent {
    public:
        virtual ~EventComponent() = default;

        virtual void Execute() = 0;
    };
}
