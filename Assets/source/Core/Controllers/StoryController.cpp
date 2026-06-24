#include "Controllers/StoryController.h"

#include "Systems/EventSystem.h"

namespace mud {
    bool StoryController::IsActive() const
    {
        return EventSystem::Instance().HasEvent();
    }

    int StoryController::GetPriority() const
    {
        return 100;
    }

    void StoryController::Update(const UpdateContext& context)
    {
        if (context.Input.Confirm)
        {
            EventSystem::Instance().PlayNext();
        }
    }
}
