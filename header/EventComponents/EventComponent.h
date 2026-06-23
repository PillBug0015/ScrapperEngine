#pragma once

namespace mud {
    class EventComponent {
    public:
        virtual ~EventComponent() = default;

        virtual void Execute() = 0;
    };
}
