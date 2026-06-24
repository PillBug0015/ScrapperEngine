#pragma once

#include <vector>
#include <string>
#include <memory>
#include "Events/EventComponent.h"

namespace mud {
    class Event
    {
    public:
        std::string UniqueID;
        int ManagedIndex = -1;
        std::vector<std::unique_ptr<EventComponent>> Components;


        ~Event() = default;
    };
}
