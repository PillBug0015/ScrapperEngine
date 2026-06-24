#pragma once

#include "Events/EventComponent.h"
#include <string>

namespace mud {
    class TextComponent : public EventComponent
    {
    public:
        std::string content;
        
        void Execute() override;
    };
}
