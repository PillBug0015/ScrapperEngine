#pragma once

#include <vector>
#include <string>
#include <memory>
#include "EventComponents\EventComponent.h"

namespace mud {
    class Event
    {
    public:
        std::string UniqueID;
        int ManagedIndex;
        std::vector<std::unique_ptr<EventComponent>> Components;


        ~Event() = default;
    };
}
