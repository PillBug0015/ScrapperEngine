#pragma once

#include "Events/EventComponent.h"

#include <vector>
#include <string>

namespace mud {
    struct ChoiceData
    {
        std::string DisplayName;
        std::string TargetEventID;
    };

    class ChoiceComponent : public EventComponent
    {
        public:
            std::vector<ChoiceData> Choices;

            void Execute() override;
    };
}
