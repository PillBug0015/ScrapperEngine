#include "Vector.h"
#include <ostream>

Vector2Int::Vector2Int(int x, int y) : x(x), y(y)
{
}

Vector2Int operator+(const Vector2Int& v1, const Vector2Int& v2)
{
    return Vector2Int(v1.x + v2.x, v1.y + v2.y);
}

std::ostream& operator<<(std::ostream& ostream, const Vector2Int& vec)
{
    ostream << "(" << vec.x << ", " << vec.y << ")";
    return ostream;
}
