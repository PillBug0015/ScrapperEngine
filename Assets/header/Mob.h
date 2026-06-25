#pragma once

#include <string>
#include <unordered_map>
#include <algorithm>

class Mob {
public:
    std::string name;
    std::unordered_map<std::string, int> stats;

    Mob() = default;
    Mob(const std::string& name) : name(name) {}

    int GetStat(const std::string& key, int defaultVal = 0) const {
        auto it = stats.find(key);
        if (it != stats.end()) {
            return it->second;
        }
        return defaultVal;
    }

    void SetStat(const std::string& key, int val) {
        stats[key] = val;
        
        // Automatic health clamping
        if (key == "HP") {
            int maxHp = GetStat("MaxHP", val);
            if (stats["HP"] > maxHp) {
                stats["HP"] = maxHp;
            }
            if (stats["HP"] < 0) {
                stats["HP"] = 0;
            }
        }
    }

    void ModifyStat(const std::string& key, int delta) {
        SetStat(key, GetStat(key) + delta);
    }

    bool IsDead() const {
        return GetStat("HP") <= 0;
    }
};
