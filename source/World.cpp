#include "World.h"

#include "JsonMapParser.h"

#include <stdexcept>
#include <utility>

World::World() {
    const std::string map_path = JsonMapParser::FindMapPath("data/map.json");
    LoadFromMapData(JsonMapParser::LoadFromFile(map_path));
}

int World::Width() const {
    if (tiles_.empty()) {
        return 0;
    }

    return static_cast<int>(tiles_.front().size());
}

int World::Height() const {
    return static_cast<int>(tiles_.size());
}

bool World::IsInside(int x, int y) const {
    return x >= 0 && y >= 0 && x < Width() && y < Height();
}

const Tile& World::GetTile(int x, int y) const {
    if (!IsInside(x, y)) {
        throw std::out_of_range("World tile position is out of range.");
    }

    return tiles_[y][x];
}

const std::vector<Entity>& World::Entities() const {
    return entities_;
}

EntityId World::PlayerId() const {
    return player_id_;
}

const Entity* World::FindEntity(EntityId id) const {
    for (const Entity& entity : entities_) {
        if (entity.Id() == id) {
            return &entity;
        }
    }

    return nullptr;
}

Entity* World::FindEntity(EntityId id) {
    for (Entity& entity : entities_) {
        if (entity.Id() == id) {
            return &entity;
        }
    }

    return nullptr;
}

const Entity* World::GetEntityAt(int x, int y) const {
    for (const Entity& entity : entities_) {
        const Position position = entity.GetPosition();
        if (position.x == x && position.y == y) {
            return &entity;
        }
    }

    return nullptr;
}

bool World::TryMoveEntity(EntityId id, int dx, int dy) {
    Entity* entity = FindEntity(id);
    if (entity == nullptr) {
        return false;
    }

    const Position current = entity->GetPosition();
    const Position next = {current.x + dx, current.y + dy};

    if (!IsInside(next.x, next.y)) {
        return false;
    }

    if (!GetTile(next.x, next.y).walkable) {
        return false;
    }

    if (HasBlockingEntityAt(next.x, next.y, id)) {
        return false;
    }

    entity->SetPosition(next);
    return true;
}

EntityId World::AddEntity(EntityType type, const std::string& name, char symbol, Position position, bool blocks_movement) {
    const EntityId id = next_entity_id_;
    ++next_entity_id_;

    entities_.emplace_back(id, type, name, symbol, position, blocks_movement);
    return id;
}

bool World::HasBlockingEntityAt(int x, int y, EntityId ignored_id) const {
    for (const Entity& entity : entities_) {
        if (entity.Id() == ignored_id || !entity.BlocksMovement()) {
            continue;
        }

        const Position position = entity.GetPosition();
        if (position.x == x && position.y == y) {
            return true;
        }
    }

    return false;
}

void World::LoadFromMapData(const JsonMapData& map_data) {
    tiles_.clear();
    entities_.clear();
    next_entity_id_ = 1;
    player_id_ = InvalidEntityId;

    tiles_.reserve(map_data.tiles.size());

    for (int y = 0; y < static_cast<int>(map_data.tiles.size()); ++y) {
        const std::string& line = map_data.tiles[y];
        std::vector<Tile> row;
        row.reserve(line.size());

        for (char symbol : line) {
            row.push_back(TileFromSymbol(symbol));
        }

        tiles_.push_back(std::move(row));
    }

    for (const EntitySpawn& spawn : map_data.entities) {
        if (!IsInside(spawn.position.x, spawn.position.y)) {
            throw std::runtime_error("Entity position is outside the map: " + spawn.name);
        }

        const EntityId id =
            AddEntity(spawn.type, spawn.name, spawn.symbol, spawn.position, spawn.blocks_movement);

        if (spawn.type == EntityType::Player) {
            player_id_ = id;
        }
    }

    if (player_id_ == InvalidEntityId) {
        player_id_ = AddEntity(EntityType::Player, "플레이어", '@', {1, 1}, true);
    }
}
