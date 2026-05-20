#pragma once

#include "Entity.h"
#include "Tile.h"

#include <string>
#include <vector>

struct JsonMapData;

class World {
public:
    World();

    int Width() const;
    int Height() const;
    bool IsInside(int x, int y) const;

    const Tile& GetTile(int x, int y) const;
    const std::vector<Entity>& Entities() const;
    EntityId PlayerId() const;

    const Entity* FindEntity(EntityId id) const;
    const Entity* GetEntityAt(int x, int y) const;

    bool TryMoveEntity(EntityId id, int dx, int dy);

private:
    Entity* FindEntity(EntityId id);
    EntityId AddEntity(EntityType type, const std::string& name, char symbol, Position position, bool blocks_movement);
    bool HasBlockingEntityAt(int x, int y, EntityId ignored_id) const;
    void LoadFromMapData(const JsonMapData& map_data);

    std::vector<std::vector<Tile>> tiles_;
    std::vector<Entity> entities_;
    EntityId next_entity_id_ = 1;
    EntityId player_id_ = InvalidEntityId;
};
