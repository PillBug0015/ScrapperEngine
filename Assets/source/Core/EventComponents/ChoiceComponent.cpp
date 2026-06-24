#include "Events/ChoiceComponent.h"
#include "Systems/ChoiceSystem.h"

namespace mud {
    void ChoiceComponent::Execute()
    {
        if (Choices.empty())
        {
            return;
        }

        ChoiceSystem::Instance().Open(Choices);
    }
}
