#include "Core/GameManager.h"

#include "Scrapper.h"
#include "System/CombatSystem/CombatSystem.h"
#include "System/EventSystem/EventSystem.h"

namespace MyMudGame {

GameManager::GameManager(EventSystem* eventSystem, CombatSystem* combatSystem)
    : eventSystem(eventSystem),
      combatSystem(combatSystem)
{
}

void GameManager::Start()
{
    Boot();
}

void GameManager::Update(float /*deltaTime*/)
{
    if (IsKeyPressed(KEY_ESCAPE))
    {
        TogglePause();
    }
}

void GameManager::Boot()
{
    if (booted)
    {
        return;
    }

    booted = true;
    database.Clear();
    database.LoadDirectory("data");

    if (eventSystem != nullptr)
    {
        eventSystem->Initialize(&database, this);
    }

    if (combatSystem != nullptr)
    {
        combatSystem->Initialize(&database, this);
    }

    SetGameState(GameState::Exploring);
}

void GameManager::RequestCombat(const CombatCommand& command)
{
    if (combatSystem != nullptr)
    {
        combatSystem->BeginCombat(command);
    }

    SetGameState(GameState::Combat);
}

void GameManager::FinishCombat()
{
    SetGameState(GameState::Exploring);
}

void GameManager::Restart()
{
    booted = false;
    DeactivateGameplaySystems();
    currentState = GameState::Exploring;
    stateBeforePause = GameState::Exploring;
    Boot();
}

void GameManager::TogglePause()
{
    if (currentState == GameState::Pause)
    {
        SetGameState(stateBeforePause);
        return;
    }

    stateBeforePause = currentState;

    if (eventSystem != nullptr && currentState == GameState::Event)
    {
        eventSystem->Pause();
    }

    if (combatSystem != nullptr && currentState == GameState::Combat)
    {
        combatSystem->Pause();
    }

    currentState = GameState::Pause;
}

void GameManager::SetGameState(GameState nextState)
{
    DeactivateGameplaySystems();
    currentState = nextState;

    switch (currentState)
    {
        case GameState::Event:
            if (eventSystem != nullptr)
            {
                eventSystem->Activate();
            }
            break;
        case GameState::Combat:
            if (combatSystem != nullptr)
            {
                combatSystem->Activate();
            }
            break;
        case GameState::Exploring:
        case GameState::Pause:
            break;
    }
}

void GameManager::DeactivateGameplaySystems()
{
    if (eventSystem != nullptr)
    {
        eventSystem->Deactivate();
    }

    if (combatSystem != nullptr)
    {
        combatSystem->Deactivate();
    }
}

}
