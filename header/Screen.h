#pragma once

struct Vector2Int;

class Screen
{
private:
public:
    Screen();
    ~Screen();

    void ClearScreen();
    void Render();
};
