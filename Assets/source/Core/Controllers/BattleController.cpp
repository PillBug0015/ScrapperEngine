#include "Controllers/BattleController.h"

#include "Systems/BattleSystem.h"

namespace mud {
    bool BattleController::IsActive() const
    {
        return BattleSystem::Instance().IsActive();
    }

    int BattleController::GetPriority() const
    {
        return 300;
    }

    void BattleController::Update(const UpdateContext& context)
    {
        BattleSystem& battleSystem = BattleSystem::Instance();

        if (context.Input.Up || context.Input.Left)
        {
            battleSystem.SelectPreviousCommand();
        }

        if (context.Input.Down || context.Input.Right)
        {
            battleSystem.SelectNextCommand();
        }

        if (context.Input.Confirm)
        {
            battleSystem.ConfirmCommand();
        }
    }
}
