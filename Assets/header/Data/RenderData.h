#pragma once

#include <string>
#include <vector>

namespace mud {
    struct StoryRenderData
    {
        std::vector<std::string> TextLines;
        std::vector<std::string> Choices;
        int SelectedChoiceIndex = 0;
        bool CanScrollUp = false;
        bool CanScrollDown = false;
    };

    struct BattleEntityRenderData
    {
        std::string Name;
        int Hp = 0;
        int MaxHp = 0;
    };

    struct BattleRenderData
    {
        std::vector<BattleEntityRenderData> Enemies;
        std::vector<BattleEntityRenderData> Players;

        std::vector<std::string> LogLines;
        std::vector<std::string> Commands;
        int SelectedCommandIndex = 0;
    };
}
