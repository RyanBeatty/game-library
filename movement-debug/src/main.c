#include <stdlib.h>

#include "raylib.h"
#include "raymath.h"

const int ScreenWidth = 800;
const int ScreenHeight = 450;

static void RunGame(float delta_time) {
    static Vector3 pos = {10, 10, 10};
    static Vector2 size = {100, 100};

    Vector3 dir = Vector3Zero();
    if (IsKeyDown(KEY_UP)) {
        dir = Vector3Add(dir, (Vector3){0, -1, 0});
    }
    if (IsKeyDown(KEY_RIGHT)) {
        dir = Vector3Add(dir, (Vector3){1, 0, 0});
    }
    if (IsKeyDown(KEY_DOWN)) {
        dir = Vector3Add(dir, (Vector3){0, 1, 0});
    }
    if (IsKeyDown(KEY_LEFT)) {
        dir = Vector3Add(dir, (Vector3){-1, 0, 0});
    }
    dir = Vector3Scale(dir, 100.0f * delta_time);
    pos = Vector3Add(pos, dir);

    BeginDrawing();

    ClearBackground(RAYWHITE);

    DrawRectangleV((Vector2){pos.x, pos.y}, size, RED);

    EndDrawing();
}

int main(void) {
    InitWindow(ScreenWidth, ScreenHeight, "Game");

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        float delta_time = GetFrameTime();
        RunGame(delta_time);
    }

    CloseWindow();

    return 0;
}