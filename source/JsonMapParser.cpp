#include "JsonMapParser.h"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace {
void ValidateTiles(const std::vector<std::string>& tiles) {
    if (tiles.empty()) {
        throw std::runtime_error("map.json must contain at least one tile row.");
    }

    const std::size_t width = tiles.front().size();
    if (width == 0) {
        throw std::runtime_error("map.json tile rows must not be empty.");
    }

    for (const std::string& row : tiles) {
        if (row.size() != width) {
            throw std::runtime_error("map.json tile rows must all have the same width.");
        }
    }
}
} // namespace

JsonMapData JsonMapParser::LoadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open map file: " + path);
    }

    nlohmann::json json;
    file >> json;

    JsonMapData map_data;
    map_data.tiles = json.at("tiles").get<std::vector<std::string>>();
    ValidateTiles(map_data.tiles);

    for (const nlohmann::json& entity_json : json.at("entities")) {
        map_data.entities.push_back(ParseEntity(entity_json));
    }

    return map_data;
}

std::string JsonMapParser::FindMapPath(const std::string& relative_path) {
    const std::filesystem::path requested_path(relative_path);
    const std::filesystem::path current_path = std::filesystem::current_path();

    const std::vector<std::filesystem::path> candidates = {
        requested_path,
        current_path / requested_path,
        current_path / ".." / requested_path,
        current_path / ".." / ".." / requested_path,
        current_path / ".." / ".." / ".." / requested_path,
    };

    for (const std::filesystem::path& candidate : candidates) {
        if (std::filesystem::exists(candidate)) {
            return candidate.string();
        }
    }

    return relative_path;
}

EntitySpawn JsonMapParser::ParseEntity(const nlohmann::json& entity_json) {
    const std::string type = entity_json.at("type").get<std::string>();
    const std::string symbol = entity_json.at("symbol").get<std::string>();

    return {
        ParseEntityType(type),
        entity_json.at("name").get<std::string>(),
        ParseSymbol(symbol),
        {entity_json.at("x").get<int>(), entity_json.at("y").get<int>()},
        entity_json.value("blocksMovement", true),
    };
}

EntityType JsonMapParser::ParseEntityType(const std::string& type) {
    if (type == "player") {
        return EntityType::Player;
    }

    if (type == "tree") {
        return EntityType::Tree;
    }

    throw std::runtime_error("Unknown entity type in map.json: " + type);
}

char JsonMapParser::ParseSymbol(const std::string& symbol) {
    if (symbol.size() != 1) {
        throw std::runtime_error("Entity symbol must be a single ASCII character.");
    }

    return symbol[0];
}
