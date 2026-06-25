#pragma once

#include "GameDatabase/GameDatabase.h"
#include "System/GameSystem.h"

namespace MyMudGame {

class GameManager;

enum class CombatState
{
    Inactive,
    Running,
    Complete
};

class CombatSystem : public GameSystem {
public:
    void Initialize(const GameDatabase* gameDatabase, GameManager* gameManager);

    void BeginCombat(const CombatCommand& command);
    void FinishCombat();
    void Update(float deltaTime) override;

    CombatState GetCombatState() const { return combatState; }

protected:
    void OnActivated() override;
    void OnDeactivated() override;
    void OnPaused() override;

private:
    const GameDatabase* database = nullptr;
    GameManager* gameManager = nullptr;
    CombatState combatState = CombatState::Inactive;
    CombatCommand currentCommand;
};

}
