#pragma once

namespace Utilities
{
    namespace RandomUtil
    {
        int GetRandomInt(int min, int max);
    }

    //헤더만 만들 생각.

    struct Vector2Int {
        int x = 0;
        int y = 0;

        constexpr Vector2Int() = default;
        constexpr Vector2Int(int x, int y) : x(x), y(y) {}

        inline Vector2Int operator+(const Vector2Int& other) const {
            return {x + other.x, y + other.y};
        };

        inline bool operator==(const Vector2Int& other) const {
            return x == other.x && y == other.y;
        };

        inline bool operator!=(const Vector2Int& other) const {
            return !(*this == other);
        };

        inline int Distance(const Vector2Int& other){
            return abs(x - other.x)+abs(y - other.y);
        }
    };
};
