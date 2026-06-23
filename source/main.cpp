#include <raylib.h>

int main()
{
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
    InitWindow(800, 450, "MyMudGame");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);
        DrawText("@", 390, 220, 32, RAYWHITE);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
