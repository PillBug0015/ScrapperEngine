#include "Tile.h"

Tile MakeTile(TileType type) {
    switch (type) {
    case TileType::Wall:
        return {type, "벽", false};
    case TileType::Water:
        return {type, "물", false};
    case TileType::Floor:
    default:
        return {type, "길", true};
    }
}

Tile TileFromSymbol(char symbol) {
    switch (symbol) {
    case '#':
        return MakeTile(TileType::Wall);
    case '~':
        return MakeTile(TileType::Water);
    case '.':
    case '@':
    case 'T':
    default:
        return MakeTile(TileType::Floor);
    }
}
