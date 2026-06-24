#pragma once

#include "Data/Entity.h"
#include "Data/RenderData.h"

#include <string>
#include <vector>

namespace mud {
    class BattleSystem
    {
    private:
        std::vector<Entity> players;
        std::vector<Entity> enemies;
        bool active = false;
        std::vector<std::string> logLines;
        std::vector<std::string> commands;
        int selectedCommandIndex = 0;

        BattleSystem() = default;

    public:
        static BattleSystem& Instance();

        void StartBattle(const std::vector<Entity>& newPlayers, const std::vector<Entity>& newEnemies);
        void EndBattle();

        void SelectPreviousCommand();
        void SelectNextCommand();
        void ConfirmCommand();

        bool IsActive() const;
        BattleRenderData GetRenderData() const;
    };
}
