#pragma once

#include <string>

using EntityId = int;

inline constexpr EntityId InvalidEntityId = -1;

struct Position {
    int x = 0;
    int y = 0;
};

enum class EntityType {
    Player,
    Tree,
};

class Entity {
public:
    Entity(EntityId id, EntityType type, std::string name, char symbol, Position position, bool blocks_movement);

    EntityId Id() const;
    EntityType Type() const;
    const std::string& Name() const;
    char Symbol() const;
    Position GetPosition() const;
    bool BlocksMovement() const;
    void SetPosition(Position position);

private:
    EntityId id_;
    EntityType type_;
    std::string name_;
    char symbol_;
    Position position_;
    bool blocks_movement_;
};
