#include "GameDatabase/GameDatabase.h"

#include <fstream>
#include <iostream>

namespace {
    bool ParseRecord(const json& record, GameDatabase& database)
    {
        if (!record.is_object())
        {
            return false;
        }

        std::string type = ToLowerHelper(record.value("type", ""));

        try {
            if (type == "entity")
            {
                database.AddEntity(record.get<EntityData>());
                return true;
            }
            if (type == "action")
            {
                database.AddAction(record.get<ActionData>());
                return true;
            }
            if (type == "item")
            {
                database.AddItem(record.get<ItemData>());
                return true;
            }
            if (type == "modifier")
            {
                database.AddModifier(record.get<ModifierData>());
                return true;
            }
            if (type == "event")
            {
                database.AddEvent(record.get<EventData>());
                return true;
            }
        } catch (const std::exception& error) {
            std::cerr << "[GameDatabase] Parsing exception: " << error.what() << "\n";
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
    events.clear(); // 추가
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

void GameDatabase::AddEvent(const EventData& data) // 추가
{
    if (!data.Header.ID.empty())
    {
        events[data.Header.ID] = data;
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

const EventData* GameDatabase::FindEvent(const std::string& id) const // 추가
{
    const auto found = events.find(id);
    return found == events.end() ? nullptr : &found->second;
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

int GameDatabase::GetEventCount() const // 추가
{
    return static_cast<int>(events.size());
}
