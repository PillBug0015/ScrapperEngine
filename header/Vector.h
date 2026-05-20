#pragma once

#include <iosfwd>


struct Vector2Int
{
    int x;
    int y;

    Vector2Int(int x, int y);
};

Vector2Int operator+(const Vector2Int& v1, const Vector2Int& v2);
std::ostream& operator<<(std::ostream& ostream, const Vector2Int& vec);
