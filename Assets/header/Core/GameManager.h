#pragma once

#include "Core/Component/Component.h"
#include "GameDatabase/GameDatabase.h"

namespace MyMudGame {

class CombatSystem;
class EventSystem;

enum class GameState
{
    Exploring,
    Event,
    Combat,
    Pause
};

class GameManager : public ScrapperEngine::Component {
public:
    GameManager(EventSystem* eventSystem, CombatSystem* combatSystem);

    void Start() override;
    void Update(float deltaTime) override;

    void Boot();
    void RequestCombat(const CombatCommand& command);
    void FinishCombat();
    void Restart();
    void TogglePause();

    GameState GetGameState() const { return currentState; }
    const GameDatabase& GetDatabase() const { return database; }

private:
    GameDatabase database;
    EventSystem* eventSystem = nullptr;
    CombatSystem* combatSystem = nullptr;
    GameState currentState = GameState::Exploring;
    GameState stateBeforePause = GameState::Exploring;
    bool booted = false;

    void SetGameState(GameState nextState);
    void DeactivateGameplaySystems();
};
}
