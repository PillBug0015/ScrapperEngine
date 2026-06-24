#pragma once

#include "Events/Event.h"

#include <memory>
#include <string>
#include <vector>

namespace mud {
    class Scene
    {
    public:
        std::string StartEventID;
        std::vector<std::unique_ptr<Event>> Events;

        void AddEvent(std::unique_ptr<Event> event)
        {
            if (event != nullptr)
            {
                Events.push_back(std::move(event));
            }
        }
    };
}
