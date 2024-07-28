#include <stdlib.h>

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

const int ScreenWidth = 1280;
const int ScreenHeight = 720;

static Camera2D GameCamera = {0};

static Shader GetShader() {
    static bool init = false;
    static Shader shader = {0};

    if (!init) {
        shader = LoadShader("shader.vs", "shader.fs");
        init = true;
    }
    return shader;
}

static void RunGame(float delta_time) {
    static Vector3 pos = {200, 200, 0};
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
    breakpoint();

    // // Need to set this every draw batch I guess.
    // Matrix view = MatrixTranslate(200, 0, 0);
    // rlSetMatrixModelview(view);
    // BeginMode2D(GameCamera);

    ClearBackground(RAYWHITE);

    // BeginShaderMode(GetShader());
    // rlMatrixMode(RL_PROJECTION);
    // rlLoadIdentity();
    // rlOrtho(0, GetScreenWidth(), GetScreenHeight(), 0, 0.0f, 1.0f);
    // rlLoadIdentity();
    // rlScalef(0.0f, -1.0f, 0.0f);

    // Ortho
    {
        // rlSetMatrixProj only.
        // TraceLog(LOG_INFO, "height %d", GetScreenHeight());
        // { rlSetMatrixProjection(MatrixOrtho(0, GetScreenWidth(), GetScreenHeight() - 400, 400, -1.0f, 1.0f)); }

        // matrix mode.
        {
            rlMatrixMode(RL_PROJECTION);
            rlLoadIdentity();
            rlOrtho(0.0, GetScreenWidth(), 0.0, GetScreenHeight(), 0.0, 1.0);

            // Switch back to modelview matrix
            rlMatrixMode(RL_MODELVIEW);
            rlLoadIdentity();
        }
    }

    breakpoint();

    Vector2 size2 = {GetScreenWidth(), GetScreenHeight()};
    Vector2 half_size = Vector2Scale(size2, 0.5f);
    DrawRectangleV((Vector2){0 - half_size.x, 0 - half_size.y}, size2, RED);
    DrawRectangleV((Vector2){GetScreenWidth() - half_size.x, 0 - half_size.y}, size2, BLUE);
    DrawRectangleV((Vector2){0 - half_size.x, GetScreenHeight() - half_size.y}, size2, GREEN);
    DrawRectangleV((Vector2){GetScreenWidth() - half_size.x, GetScreenHeight() - half_size.y}, size2, YELLOW);

    DrawRectangleV((Vector2){pos.x, pos.y}, size, BLUE);

    // EndShaderMode();
    EndMode2D();

    EndDrawing();
}

int main(void) {
    InitWindow(ScreenWidth, ScreenHeight, "Game");

    GameCamera.target = (Vector2){0.0f, 0.0f};
    GameCamera.offset = (Vector2){20.0f, 0.0f};
    GameCamera.rotation = 0.0f;
    GameCamera.zoom = 1.0f;

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        float delta_time = GetFrameTime();
        RunGame(delta_time);
    }

    CloseWindow();

    return 0;
}