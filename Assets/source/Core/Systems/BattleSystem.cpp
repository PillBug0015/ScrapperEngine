#include "Systems/BattleSystem.h"

#include "Data/StatKey.h"

#include <algorithm>

namespace mud {
    namespace {
        BattleEntityRenderData ToBattleEntityRenderData(const Entity& entity)
        {
            BattleEntityRenderData data;
            data.Name = entity.GetDisplayName();
            data.Hp = entity.GetStat(StatKey::Hp);
            data.MaxHp = entity.GetStat(StatKey::MaxHp);
            return data;
        }
    }

    BattleSystem& BattleSystem::Instance()
    {
        static BattleSystem instance;
        return instance;
    }

    void BattleSystem::StartBattle(const std::vector<Entity>& newPlayers, const std::vector<Entity>& newEnemies)
    {
        players = newPlayers;
        enemies = newEnemies;
        active = true;
        selectedCommandIndex = 0;

        logLines.clear();

        if (enemies.empty())
        {
            logLines.push_back("No enemies appear.");
        }
        else if (enemies.size() == 1)
        {
            logLines.push_back(enemies.front().GetDisplayName() + " appears.");
        }
        else
        {
            logLines.push_back("Enemies appear.");
        }

        commands = players.empty() ? std::vector<std::string>() : players.front().GetActionIDs();
    }

    void BattleSystem::EndBattle()
    {
        active = false;
        players.clear();
        enemies.clear();
        logLines.clear();
        commands.clear();
        selectedCommandIndex = 0;
    }

    void BattleSystem::SelectPreviousCommand()
    {
        if (!active || commands.empty())
        {
            return;
        }

        selectedCommandIndex = std::max(0, selectedCommandIndex - 1);
    }

    void BattleSystem::SelectNextCommand()
    {
        if (!active || commands.empty())
        {
            return;
        }

        selectedCommandIndex = std::min(static_cast<int>(commands.size()) - 1, selectedCommandIndex + 1);
    }

    void BattleSystem::ConfirmCommand()
    {
        if (!active || commands.empty())
        {
            return;
        }

        logLines.push_back(commands[selectedCommandIndex] + " is not implemented yet.");
    }

    bool BattleSystem::IsActive() const
    {
        return active;
    }

    BattleRenderData BattleSystem::GetRenderData() const
    {
        BattleRenderData data;

        for (const Entity& enemy : enemies)
        {
            data.Enemies.push_back(ToBattleEntityRenderData(enemy));
        }

        for (const Entity& player : players)
        {
            data.Players.push_back(ToBattleEntityRenderData(player));
        }

        data.LogLines = logLines;
        data.Commands = commands;
        data.SelectedCommandIndex = selectedCommandIndex;
        return data;
    }
}
