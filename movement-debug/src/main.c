#include <stdlib.h>

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

int main(void) {
    InitWindow(1280, 720, "Game");
    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);

        rlMatrixMode(RL_PROJECTION);
        rlLoadIdentity();

        // This works and things display fine.
        rlOrtho(0.0f, (float)GetScreenWidth(), (float)GetScreenHeight(), 0.0f, -1.0f, 1.0f);
        // This does not work and nothing shows up.
        // rlOrtho(0.0f, (float)GetScreenWidth(), 0.0f, (float)GetScreenHeight(), -1.0f, 1.0f);

        rlMatrixMode(RL_MODELVIEW);
        rlLoadIdentity();

        // Draw some debug background recs for visual debugging. Each rec will be centered at a corner.
        Vector2 size = {GetScreenWidth(), GetScreenHeight()};
        Vector2 half_size = Vector2Scale(size, 0.5f);
        DrawRectangleV((Vector2){0 - half_size.x, 0 - half_size.y}, size, RED);
        DrawRectangleV((Vector2){GetScreenWidth() - half_size.x, 0 - half_size.y}, size, BLUE);
        DrawRectangleV((Vector2){0 - half_size.x, GetScreenHeight() - half_size.y}, size, GREEN);
        DrawRectangleV((Vector2){GetScreenWidth() - half_size.x, GetScreenHeight() - half_size.y}, size, YELLOW);

        EndMode2D();
        EndDrawing();
    }
    CloseWindow();
    return 0;
}