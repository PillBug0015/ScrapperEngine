#include "Controllers/ChoiceController.h"

#include "Systems/ChoiceSystem.h"
#include "Systems/EventSystem.h"

#include <string>

namespace mud {
    bool ChoiceController::IsActive() const
    {
        return ChoiceSystem::Instance().IsOpen();
    }

    int ChoiceController::GetPriority() const
    {
        return 200;
    }

    void ChoiceController::Update(const UpdateContext& context)
    {
        ChoiceSystem& choiceSystem = ChoiceSystem::Instance();

        if (context.Input.Up)
        {
            choiceSystem.MoveUp();
        }

        if (context.Input.Down)
        {
            choiceSystem.MoveDown();
        }

        if (context.Input.Confirm)
        {
            const std::string targetEventID = choiceSystem.Confirm();

            if (targetEventID.empty())
            {
                return;
            }

            EventSystem::Instance().JumpTo(targetEventID);
            EventSystem::Instance().PlayNext();
        }
    }
}
