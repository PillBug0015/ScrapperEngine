#pragma once

#include "EventComponents\EventComponent.h"
#include <string>


#pragma once
namespace mud {
    class TextComponent : public EventComponent
    {
    public:
        std::string content;
        
        virtual void Execute() override;
    };
}