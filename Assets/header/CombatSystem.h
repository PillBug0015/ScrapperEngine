#pragma once

#include "Mob.h"
#include <memory>
#include <string>
#include <vector>

class GameManager;

class CombatSystem {
public:
    CombatSystem() = default;
    ~CombatSystem() = default;

    void StartCombat(std::unique_ptr<Mob> enemyMob, Mob* playerMob);
    void Update(float deltaTime, GameManager* game);
    void Draw(GameManager* game);

    void PlayerAttack(GameManager* game);
    void PlayerHeal(GameManager* game);
    void PlayerRun(GameManager* game);

    bool IsCombatFinished() const { return combatFinished; }
    bool DidPlayerWin() const { return playerWon; }

private:
    Mob* player = nullptr;
    std::unique_ptr<Mob> enemy;
    
    bool isPlayerTurn = true;
    bool combatFinished = false;
    bool playerWon = false;
    
    float turnTimer = 0.0f;
    bool waitingForEnemy = false;

    std::vector<std::string> battleLogs;
    int selectedMenuIndex = 0; // For JRPG menu navigation

    void AddBattleLog(const std::string& log);
    void EnemyTurn(GameManager* game);
};
