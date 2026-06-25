#pragma once

#include "Renderer.h"
#include "Mob.h"
#include "EventSystem.h"
#include "StorySystem.h"
#include "CombatSystem.h"
#include "Controller.h"
#include <vector>
#include <string>

enum class GameState {
    STORY_MODE,
    COMBAT_MODE,
    GAME_OVER
};

class GameManager : public ScrapperEngine::Renderer {
public:
    GameManager();
    ~GameManager() override = default;

    // Prevent copying
    GameManager(const GameManager&) = delete;
    GameManager& operator=(const GameManager&) = delete;

    // Component lifecycle
    void Start() override;
    void Update(float deltaTime) override;

    void TransitionTo(GameState newState);
    void AddLog(const std::string& log);
    void ClearLogs();
    
    // Getters
    Mob& GetPlayer() { return player; }
    EventSystem& GetEventSystem() { return eventSystem; }
    StorySystem& GetStorySystem() { return storySystem; }
    CombatSystem& GetCombatSystem() { return combatSystem; }
    GameState GetState() const { return state; }
    const std::vector<std::string>& GetLogs() const { return gameLogs; }

private:
    GameState state = GameState::STORY_MODE;
    
    Mob player;
    EventSystem eventSystem;
    StorySystem storySystem;
    CombatSystem combatSystem;
    Controller controller;

    std::vector<std::string> gameLogs;

protected:
    void SubmitRenderCommand() override;
    
private:
    void DrawHeader();
    void DrawLogs();
    void DrawGameOver();
};
