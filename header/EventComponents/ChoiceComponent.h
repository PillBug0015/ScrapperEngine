#pragma once

#include "EventComponents/EventComponent.h"

#include <vector>
#include <string>

namespace mud {

    struct ChoiceData
    {
        std::string DisplayName;
        int TargetIndex;
    };

    class ChoiceComponent : public EventComponent
    {
        public:
            std::vector<ChoiceData> Choices;

            virtual void Execute() override;
    };
}
