#pragma once

#include <string>

enum class TileType {
    Floor,
    Wall,
    Water,
};

struct Tile {
    TileType type;
    std::string name;
    bool walkable;
};

Tile MakeTile(TileType type);
Tile TileFromSymbol(char symbol);
