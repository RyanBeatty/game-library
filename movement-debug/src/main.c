#include <stdlib.h>

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

int main(void) {
    InitWindow(1280, 720, "Game");

    float size = 100.0f;  // size of the triangle

    Vector2 pos = {0.0f - (size * 0.5f), 0.0f - (size * 0.5f)};

    Camera2D camera = {0};
    // camera.target = (Vector2){0.0f, 0.0f};
    camera.target = pos;
    camera.offset = (Vector2){GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};
    camera.rotation = 0.0f;  // in deg.
    camera.zoom = 1.0f;

    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        Vector2 dir = Vector2Zero();
        Matrix view_m = GetCameraMatrix2D(camera);

        if (IsKeyDown(KEY_RIGHT)) {
            dir = Vector2Add(dir, (Vector2){1.0, 0.0});
        }
        if (IsKeyDown(KEY_LEFT)) {
            dir = Vector2Add(dir, (Vector2){-1.0, 0.0});
        }
        if (IsKeyDown(KEY_UP)) {
            dir = Vector2Add(dir, (Vector2){0.0f, 1.0f});
            breakpoint();
        }
        if (IsKeyDown(KEY_DOWN)) {
            dir = Vector2Add(dir, (Vector2){0.0f, -1.0f});
        }

        pos = Vector2Add(pos, Vector2Scale(dir, 10.0f));

        camera.target = pos;

        if (IsKeyDown(KEY_A)) {
            camera.rotation += 1.0f;
        }
        if (IsKeyDown(KEY_S)) {
            camera.rotation -= 1.0f;
        }

        BeginDrawing();

        ClearBackground(RAYWHITE);

        BeginMode2D(camera);

        // Draw some debug background recs for visual debugging. Each rec will be centered at a corner.
        Vector2 size = {GetScreenWidth(), GetScreenHeight()};
        Vector2 half_size = Vector2Scale(size, 0.5f);
        DrawRectangleV((Vector2){0 - half_size.x, 0 - half_size.y}, size, RED);
        DrawRectangleV((Vector2){GetScreenWidth() - half_size.x, 0 - half_size.y}, size, BLUE);
        DrawRectangleV((Vector2){0 - half_size.x, GetScreenHeight() - half_size.y}, size, GREEN);
        DrawRectangleV((Vector2){GetScreenWidth() - half_size.x, GetScreenHeight() - half_size.y}, size, YELLOW);

        DrawRectangleV(pos, (Vector2){100, 100}, BLACK);

        EndMode2D();

        EndDrawing();
    }
    CloseWindow();
    return 0;
}