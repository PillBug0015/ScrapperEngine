#include "Data/Entity.h"

namespace mud {
    Entity::Entity(const EntityData& data)
        : dataID(data.ID),
          displayName(data.DisplayName),
          stats(data.Stats),
          actionIDs(data.ActionIDs),
          modifierIDs(data.ModifierIDs)
    {
    }

    const std::string& Entity::GetDataID() const
    {
        return dataID;
    }

    const std::string& Entity::GetDisplayName() const
    {
        return displayName;
    }

    int Entity::GetStat(const std::string& key) const
    {
        const auto foundStat = stats.find(key);

        if (foundStat == stats.end())
        {
            return 0;
        }

        return foundStat->second;
    }

    void Entity::SetStat(const std::string& key, int value)
    {
        stats[key] = value;
    }

    void Entity::AddStat(const std::string& key, int amount)
    {
        SetStat(key, GetStat(key) + amount);
    }

    const std::vector<std::string>& Entity::GetActionIDs() const
    {
        return actionIDs;
    }

    const std::vector<std::string>& Entity::GetModifierIDs() const
    {
        return modifierIDs;
    }
}
