#include <raylib.h>
#include <iostream>
#include <vector>

const float TILE_SIZE = 2;
const float TILE_SIZE_X = 24;
const float TILE_SIZE_Y = 24;

struct MapData
{
    int width;
    int height;
    std::vector<int> map;

    int GetTile(int x, int y) const
    { 
        if (x < 0 || x >= width || y < 0 || y >= height) return -1;
        return map[y * width + x]; 
    }

    MapData(int width, int height) : width(width), height(height), map(width * height, ' ')
    {
    }
};


void DrawMap(const MapData& mapData)
{
    int currentX = 0;
    int currentY = 0;

    for(int y = 0; y < mapData.height; y++)
    {
        for(int x = 0; x < mapData.width; x++)
        {
            currentX = TILE_SIZE_X * x;
            currentY = TILE_SIZE_Y * y;

            int c = mapData.GetTile(x, y);

            const char* cp = TextFormat("%d", c);
            DrawText(cp, currentX, currentY, TILE_SIZE, RAYWHITE);
        }
    }
}

int main()
{
    InitWindow(800, 450, "MyMudGame");
    SetTargetFPS(30);
    
    int width = 16;
    int height = 16;
    MapData map(width, height);

    for(int i = 0; i < width * height; i++)
    {
        map.map[i] = i;
    }  

    while (!WindowShouldClose()) {
        BeginDrawing();
        DrawMap(map);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
