#include "Entity.h"

#include <utility>

Entity::Entity(EntityId id, EntityType type, std::string name, char symbol, Position position, bool blocks_movement)
    : id_(id),
      type_(type),
      name_(std::move(name)),
      symbol_(symbol),
      position_(position),
      blocks_movement_(blocks_movement) {}

EntityId Entity::Id() const {
    return id_;
}

EntityType Entity::Type() const {
    return type_;
}

const std::string& Entity::Name() const {
    return name_;
}

char Entity::Symbol() const {
    return symbol_;
}

Position Entity::GetPosition() const {
    return position_;
}

bool Entity::BlocksMovement() const {
    return blocks_movement_;
}

void Entity::SetPosition(Position position) {
    position_ = position;
}
