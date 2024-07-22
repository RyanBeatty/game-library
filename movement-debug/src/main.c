#include <stdlib.h>

#include "raylib.h"
#include "raymath.h"

const int ScreenWidth = 800;
const int ScreenHeight = 450;

static void Draw() {
    BeginDrawing();

    ClearBackground(RAYWHITE);

    EndDrawing();
}

static void Update(float delta_time) {}

int main(void) {
    InitWindow(ScreenWidth, ScreenHeight, "Game");

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        float delta_time = GetFrameTime();
        Update(delta_time);
        Draw();
    }

    CloseWindow();

    return 0;
}