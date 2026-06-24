#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace mud {
    struct EntityData
    {
        std::string ID;
        std::string DisplayName;
        std::unordered_map<std::string, int> Stats;
        std::vector<std::string> ActionIDs;
        std::vector<std::string> ModifierIDs;
    };
}
