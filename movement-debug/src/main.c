#include <stdlib.h>

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

int main(void) {
    InitWindow(1280, 720, "Game");

    Vector2 pos = {200, 200};

    RenderTexture target = LoadRenderTexture(1280, 720);
    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        if (IsKeyDown(KEY_RIGHT)) {
            pos = Vector2Add(pos, (Vector2){10.0, 0.0});
        }

        if (IsKeyDown(KEY_LEFT)) {
            pos = Vector2Add(pos, (Vector2){-10.0, 0.0});
        }
        if (IsKeyDown(KEY_UP)) {
            pos = Vector2Add(pos, (Vector2){0.0f, 10.0f});
        }
        if (IsKeyDown(KEY_DOWN)) {
            pos = Vector2Add(pos, (Vector2){0.0f, -10.0f});
        }

        // BeginTextureMode(target);
        BeginDrawing();
        ClearBackground(RAYWHITE);

        // rlMatrixMode(RL_PROJECTION);
        // rlLoadIdentity();

        // // This works and things display fine.
        // // rlOrtho(0.0f, (float)GetScreenWidth(), (float)GetScreenHeight(), 0.0f, -1.0f, 1.0f);
        // // This does not work and nothing shows up.
        // // rlOrtho(0.0f, (float)GetScreenWidth(), 0.0f, (float)GetScreenHeight(), -1.0f, 1.0f);

        // rlMatrixMode(RL_MODELVIEW);
        // rlLoadIdentity();

        // Draw some debug background recs for visual debugging. Each rec will be centered at a corner.
        Vector2 size = {GetScreenWidth(), GetScreenHeight()};
        Vector2 half_size = Vector2Scale(size, 0.5f);
        DrawRectangleV((Vector2){0 - half_size.x, 0 - half_size.y}, size, RED);
        DrawRectangleV((Vector2){GetScreenWidth() - half_size.x, 0 - half_size.y}, size, BLUE);
        DrawRectangleV((Vector2){0 - half_size.x, GetScreenHeight() - half_size.y}, size, GREEN);
        DrawRectangleV((Vector2){GetScreenWidth() - half_size.x, GetScreenHeight() - half_size.y}, size, YELLOW);

        DrawRectangleV(pos, (Vector2){100, 100}, BLACK);

        // EndTextureMode();

        // BeginDrawing();

        // ClearBackground(WHITE);
        // Texture texture = target.texture;
        // DrawTexturePro(texture, (Rectangle){0, 0, texture.width, -texture.height},
        //                (Rectangle){0, 0, texture.width, texture.height}, (Vector2){0.0f, 0.0f}, 0.0f, WHITE);
        EndDrawing();
    }
    CloseWindow();
    return 0;
}