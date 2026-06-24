#include "Data/GameDatabase.h"

namespace mud {
    GameDatabase& GameDatabase::Instance()
    {
        static GameDatabase instance;
        return instance;
    }

    void GameDatabase::Clear()
    {
        entityDataByID.clear();
    }

    void GameDatabase::AddEntityData(const EntityData& data)
    {
        entityDataByID[data.ID] = data;
    }

    const EntityData* GameDatabase::FindEntityData(const std::string& id) const
    {
        const auto foundData = entityDataByID.find(id);

        if (foundData == entityDataByID.end())
        {
            return nullptr;
        }

        return &foundData->second;
    }

    std::optional<Entity> GameDatabase::CreateEntity(const std::string& id) const
    {
        const EntityData* data = FindEntityData(id);

        if (data == nullptr)
        {
            return std::nullopt;
        }

        return Entity(*data);
    }
}
