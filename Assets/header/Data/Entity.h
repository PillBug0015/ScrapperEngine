#pragma once

#include "Data/EntityData.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace mud {
    class Entity
    {
    private:
        std::string dataID;
        std::string displayName;
        std::unordered_map<std::string, int> stats;
        std::vector<std::string> actionIDs;
        std::vector<std::string> modifierIDs;

    public:
        Entity() = default;
        explicit Entity(const EntityData& data);

        const std::string& GetDataID() const;
        const std::string& GetDisplayName() const;

        int GetStat(const std::string& key) const;
        void SetStat(const std::string& key, int value);
        void AddStat(const std::string& key, int amount);

        const std::vector<std::string>& GetActionIDs() const;
        const std::vector<std::string>& GetModifierIDs() const;
    };
}
