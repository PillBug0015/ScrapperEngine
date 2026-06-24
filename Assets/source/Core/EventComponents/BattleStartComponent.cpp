#include "Events/BattleStartComponent.h"

#include "Data/GameDatabase.h"
#include "Systems/BattleSystem.h"
#include "Systems/TextSystem.h"

#include <optional>
#include <vector>

namespace mud {
    void BattleStartComponent::Execute()
    {
        std::vector<Entity> players;
        std::vector<Entity> enemies;

        for (const std::string& playerEntityID : PlayerEntityIDs)
        {
            const std::optional<Entity> player = GameDatabase::Instance().CreateEntity(playerEntityID);

            if (!player.has_value())
            {
                TextSystem::Instance().AddText("Battle data is missing.");
                return;
            }

            players.push_back(player.value());
        }

        for (const std::string& enemyEntityID : EnemyEntityIDs)
        {
            const std::optional<Entity> enemy = GameDatabase::Instance().CreateEntity(enemyEntityID);

            if (!enemy.has_value())
            {
                TextSystem::Instance().AddText("Battle data is missing.");
                return;
            }

            enemies.push_back(enemy.value());
        }

        if (players.empty() || enemies.empty())
        {
            TextSystem::Instance().AddText("Battle data is missing.");
            return;
        }

        BattleSystem::Instance().StartBattle(players, enemies);
    }
}
