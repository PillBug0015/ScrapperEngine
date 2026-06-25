#include "System/CombatSystem/CombatSystem.h"

#include "Core/GameManager.h"

namespace MyMudGame {

void CombatSystem::Initialize(const GameDatabase* gameDatabase, GameManager* manager)
{
    database = gameDatabase;
    gameManager = manager;
}

void CombatSystem::BeginCombat(const CombatCommand& command)
{
    currentCommand = command;
    combatState = CombatState::Running;
}

void CombatSystem::FinishCombat()
{
    combatState = CombatState::Complete;

    if (gameManager != nullptr)
    {
        gameManager->FinishCombat();
    }
}

void CombatSystem::Update(float /*deltaTime*/)
{
}

void CombatSystem::OnActivated()
{
}

void CombatSystem::OnDeactivated()
{
    combatState = CombatState::Inactive;
}

void CombatSystem::OnPaused()
{
}

}
