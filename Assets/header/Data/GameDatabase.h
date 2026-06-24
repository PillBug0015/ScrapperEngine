#pragma once

#include "Data/Entity.h"
#include "Data/EntityData.h"

#include <optional>
#include <string>
#include <unordered_map>

namespace mud {
    class GameDatabase
    {
    private:
        std::unordered_map<std::string, EntityData> entityDataByID;

        GameDatabase() = default;

    public:
        static GameDatabase& Instance();

        void Clear();
        void AddEntityData(const EntityData& data);

        const EntityData* FindEntityData(const std::string& id) const;
        std::optional<Entity> CreateEntity(const std::string& id) const;
    };
}
