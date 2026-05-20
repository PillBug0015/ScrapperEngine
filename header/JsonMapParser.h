#pragma once

#include "Entity.h"

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

struct EntitySpawn {
    EntityType type;
    std::string name;
    char symbol;
    Position position;
    bool blocks_movement;
};

struct JsonMapData {
    std::vector<std::string> tiles;
    std::vector<EntitySpawn> entities;
};

class JsonMapParser {
public:
    static JsonMapData LoadFromFile(const std::string& path);
    static std::string FindMapPath(const std::string& relative_path);

private:
    static EntitySpawn ParseEntity(const nlohmann::json& entity_json);
    static EntityType ParseEntityType(const std::string& type);
    static char ParseSymbol(const std::string& symbol);
};
