#include "GameDatabase/GameDatabase.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace {
    std::string ToLower(std::string value)
    {
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });
        return value;
    }

    std::string ReadString(const json& data, const std::vector<std::string>& keys, const std::string& fallback = "")
    {
        for (const std::string& key : keys)
        {
            if (data.contains(key) && data[key].is_string())
            {
                return data[key].get<std::string>();
            }
        }

        return fallback;
    }

    int ReadInt(const json& data, const std::vector<std::string>& keys, int fallback = 0)
    {
        for (const std::string& key : keys)
        {
            if (data.contains(key) && data[key].is_number_integer())
            {
                return data[key].get<int>();
            }
        }

        return fallback;
    }

    float ReadFloat(const json& data, const std::vector<std::string>& keys, float fallback = 0.0f)
    {
        for (const std::string& key : keys)
        {
            if (data.contains(key) && data[key].is_number())
            {
                return data[key].get<float>();
            }
        }

        return fallback;
    }

    std::vector<std::string> ReadStringArray(const json& data, const std::string& key)
    {
        std::vector<std::string> values;

        if (!data.contains(key) || !data[key].is_array())
        {
            return values;
        }

        for (const auto& item : data[key])
        {
            if (item.is_string())
            {
                values.push_back(item.get<std::string>());
            }
        }

        return values;
    }

    DataHeader ParseHeader(const json& data)
    {
        DataHeader header;
        header.ID = ReadString(data, { "id", "uniqueID" });
        header.DisplayName = ReadString(data, { "displayName", "name", "displayInitialName" });
        return header;
    }

    StatType ParseStatType(const std::string& value)
    {
        const std::string normalized = ToLower(value);

        if (normalized == "intelligence" || normalized == "int") {
            return StatType::Intelligence;
        }

        if (normalized == "mobility" || normalized == "mob") {
            return StatType::Mobility;
        }

        return StatType::Strength;
    }

    TargetSide ParseTargetSide(const std::string& value)
    {
        const std::string normalized = ToLower(value);

        if (normalized == "self") {
            return TargetSide::Self;
        }

        if (normalized == "ally" || normalized == "allies") {
            return TargetSide::Ally;
        }

        return TargetSide::Enemy;
    }

    TargetMode ParseTargetMode(const std::string& value)
    {
        const std::string normalized = ToLower(value);

        if (normalized == "allside" || normalized == "side") {
            return TargetMode::AllSide;
        }

        if (normalized == "all") {
            return TargetMode::All;
        }

        return TargetMode::Single;
    }

    ModifierType ParseModifierType(const std::string& value)
    {
        return ToLower(value) == "debuff" ? ModifierType::Debuff : ModifierType::Buff;
    }

    ActionCommand ParseActionCommand(const json& data)
    {
        const std::string type = ToLower(ReadString(data, { "type", "commandType" }));

        if (type == "selecttarget" || type == "target")
        {
            return { SelectTargetCommand{
                ParseTargetSide(ReadString(data, { "side" }, "enemy")),
                ParseTargetMode(ReadString(data, { "mode" }, "single"))
            } };
        }

        if (type == "dealdamage" || type == "damage")
        {
            return { DealDamageCommand{
                ReadInt(data, { "flatAmount", "amount", "damage" }, 0),
                ParseStatType(ReadString(data, { "stat", "statKey" }, "strength")),
                ReadFloat(data, { "scale" }, 1.0f)
            } };
        }

        if (type == "heal")
        {
            return { HealCommand{ ReadInt(data, { "amount", "heal" }, 0) } };
        }

        if (type == "addmodifier")
        {
            return { AddModifierCommand{ ReadString(data, { "modifierId", "modifierID", "id" }) } };
        }

        if (type == "removemodifier")
        {
            return { RemoveModifierCommand{ ReadString(data, { "modifierId", "modifierID", "id" }) } };
        }

        return { PrintLogCommand{ ReadString(data, { "text", "log" }) } };
    }

    EntityData ParseEntity(const json& data)
    {
        EntityData entity;
        entity.Header = ParseHeader(data);

        const json& stats = data.contains("stats") && data["stats"].is_object() ? data["stats"] : data;

        entity.InitialMaxHealthPoint = ReadInt(stats, { "maxHealthPoint", "maxHp", "maxHP" }, 0);
        entity.InitialHealthPoint = ReadInt(stats, { "healthPoint", "hp", "HP" }, entity.InitialMaxHealthPoint);
        entity.InitialStrength = ReadInt(stats, { "strength", "str" }, 0);
        entity.InitialIntelligence = ReadInt(stats, { "intelligence", "int" }, 0);
        entity.InitialMobility = ReadInt(stats, { "mobility", "mob" }, 0);
        entity.ActionIDs = ReadStringArray(data, "actionIds");

        return entity;
    }

    ActionData ParseAction(const json& data)
    {
        ActionData action;
        action.Header = ParseHeader(data);
        action.APCost = ReadInt(data, { "apCost", "mpCost" }, 0);

        if (data.contains("commands") && data["commands"].is_array())
        {
            for (const auto& command : data["commands"])
            {
                if (command.is_object())
                {
                    action.Commands.push_back(ParseActionCommand(command));
                }
            }
        }
        else if (data.contains("damage"))
        {
            action.Commands.push_back({ SelectTargetCommand{} });
            action.Commands.push_back({ PrintLogCommand{ "{user} uses {action}!" } });
            action.Commands.push_back({ DealDamageCommand{ ReadInt(data, { "damage" }, 0), StatType::Strength, 1.0f } });
        }

        return action;
    }

    ItemData ParseItem(const json& data)
    {
        ItemData item;
        item.Header = ParseHeader(data);
        item.Price = ReadInt(data, { "price" }, 0);
        return item;
    }

    ModifierData ParseModifier(const json& data)
    {
        ModifierData modifier;
        modifier.Header = ParseHeader(data);
        modifier.Type = ParseModifierType(ReadString(data, { "modifierType", "type" }, "buff"));
        return modifier;
    }

    bool ParseRecord(const json& record, GameDatabase& database)
    {
        if (!record.is_object())
        {
            return false;
        }

        const std::string type = ToLower(ReadString(record, { "type" }));

        if (type == "entity")
        {
            database.AddEntity(ParseEntity(record));
            return true;
        }

        if (type == "action")
        {
            database.AddAction(ParseAction(record));
            return true;
        }

        if (type == "item")
        {
            database.AddItem(ParseItem(record));
            return true;
        }

        if (type == "modifier")
        {
            database.AddModifier(ParseModifier(record));
            return true;
        }

        return false;
    }
}

void GameDatabase::Clear()
{
    entities.clear();
    actions.clear();
    items.clear();
    modifiers.clear();
}

bool GameDatabase::LoadFile(const std::filesystem::path& path)
{
    std::ifstream file(path);

    if (!file.is_open())
    {
        return false;
    }

    json root;

    try
    {
        file >> root;
    }
    catch (const json::exception& error)
    {
        std::cerr << "Failed to parse JSON: " << path << " (" << error.what() << ")\n";
        return false;
    }

    bool loadedAnyRecord = false;

    if (root.is_object() && root.contains("records") && root["records"].is_array())
    {
        for (const auto& record : root["records"])
        {
            loadedAnyRecord = ParseRecord(record, *this) || loadedAnyRecord;
        }

        return loadedAnyRecord;
    }

    if (root.is_object())
    {
        return ParseRecord(root, *this);
    }

    if (root.is_array())
    {
        for (const auto& record : root)
        {
            loadedAnyRecord = ParseRecord(record, *this) || loadedAnyRecord;
        }
    }

    return loadedAnyRecord;
}

int GameDatabase::LoadDirectory(const std::filesystem::path& directory)
{
    if (!std::filesystem::exists(directory))
    {
        return 0;
    }

    int loadedFileCount = 0;

    for (const auto& entry : std::filesystem::recursive_directory_iterator(directory))
    {
        if (!entry.is_regular_file() || entry.path().extension() != ".json")
        {
            continue;
        }

        if (LoadFile(entry.path()))
        {
            loadedFileCount++;
        }
    }

    return loadedFileCount;
}

void GameDatabase::AddEntity(const EntityData& data)
{
    if (!data.Header.ID.empty())
    {
        entities[data.Header.ID] = data;
    }
}

void GameDatabase::AddAction(const ActionData& data)
{
    if (!data.Header.ID.empty())
    {
        actions[data.Header.ID] = data;
    }
}

void GameDatabase::AddItem(const ItemData& data)
{
    if (!data.Header.ID.empty())
    {
        items[data.Header.ID] = data;
    }
}

void GameDatabase::AddModifier(const ModifierData& data)
{
    if (!data.Header.ID.empty())
    {
        modifiers[data.Header.ID] = data;
    }
}

const EntityData* GameDatabase::FindEntity(const std::string& id) const
{
    const auto found = entities.find(id);
    return found == entities.end() ? nullptr : &found->second;
}

const ActionData* GameDatabase::FindAction(const std::string& id) const
{
    const auto found = actions.find(id);
    return found == actions.end() ? nullptr : &found->second;
}

const ItemData* GameDatabase::FindItem(const std::string& id) const
{
    const auto found = items.find(id);
    return found == items.end() ? nullptr : &found->second;
}

const ModifierData* GameDatabase::FindModifier(const std::string& id) const
{
    const auto found = modifiers.find(id);
    return found == modifiers.end() ? nullptr : &found->second;
}

int GameDatabase::GetEntityCount() const
{
    return static_cast<int>(entities.size());
}

int GameDatabase::GetActionCount() const
{
    return static_cast<int>(actions.size());
}

int GameDatabase::GetItemCount() const
{
    return static_cast<int>(items.size());
}

int GameDatabase::GetModifierCount() const
{
    return static_cast<int>(modifiers.size());
}
