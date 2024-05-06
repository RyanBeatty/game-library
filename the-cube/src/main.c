
#ifndef UNITY_BUILD
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "entity.c"
#include "level.c"
#include "npc.c"
//
#include "transaction.c"
//
#include "player.c"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "serde.c"
//
#include "editor.c"
//
#include "camera.c"
#include "ui.c"
#endif

bool GameUpdateEnabled = true;
bool GameDebugEnabled = false;

static void EnableGameUpdate() { GameUpdateEnabled = true; }

static void DisableGameUpdate() { GameUpdateEnabled = false; }

/////////////////////////////////////////////////////////////////////////////////////

static void GameInit() {
    InitWindow(ScreenWidth, ScreenHeight, "Game");

    SetTargetFPS(60);

    LoadLevel("level_1");
}

static void GameUpdate(float delta_time) {
    if (!GameUpdateEnabled) {
        return;
    }

    Shader* shader = GetShader();
    float cameraPos[3] = {GameCamera.position.x, GameCamera.position.y, GameCamera.position.z};
    SetShaderValue(*shader, shader->locs[SHADER_LOC_VECTOR_VIEW], cameraPos, SHADER_UNIFORM_VEC3);

    LevelData* level = GetCurLevel();
    if (level->finished || level->failed) {
        // TODO: Consider moving this call into LoadLevel.
        TransactionsReset();
        if (IsKeyPressed(KEY_ENTER)) {
            LoadLevel(level->name);
        } else if (IsKeyPressed(KEY_N)) {
            char* next_level = LevelDataGetNextLevelName(level);
            LoadLevel(next_level);
        }
    } else if (IsKeyPressed(KEY_U)) {
        TransactionUndo();
    } else {
        for (uint32_t i = 0; i < LevelDataNumEntities(level); ++i) {
            Entity* entity = LevelDataGetEntity(level, i);
            EntityUpdate(entity, delta_time);
        }
    }
}

static void GameDraw(float delta_time) {
    LevelData* level = GetCurLevel();

    BeginMode3D(GameCamera);

    Vector3 position = {0};
    for (uint32_t i = 0; i < LevelDataNumEntities(level); ++i) {
        Entity* entity = LevelDataGetEntity(level, i);
        DrawModel(entity->model, position, 1, WHITE);
    }

    EndMode3D();

    if (level->finished) {
        char text[256] = {0};
        sprintf(text, "Level Completed! Press Enter to Replay");
        DrawText(text, 20 + 5, 20 + 5, 18, BLACK);
    } else if (level->failed) {
        char text[256] = {0};
        sprintf(text, "Level Failed! Press Enter to Replay");
        DrawText(text, 20 + 5, 20 + 5, 18, BLACK);
    }
}

int main(void) {
    GameInit();

    Camera3D* camera = GetGameCamera();
    camera->position = (Vector3){0, 10, 0};
    camera->position = (Vector3){0, 30, 0.0f};     // Position above the scene
    camera->target = (Vector3){0.0f, 0.0f, 0.0f};  // Target at the center of the scene
    camera->up = (Vector3){0.0f, 0.0f, 1.0f};      // Up vector (positive Y-axis)
    camera->fovy = 20.0f;
    camera->projection = CAMERA_ORTHOGRAPHIC;

    // {
    //     FilePathList files = LoadDirectoryFiles("resources/levels");
    //     for (uint32_t i = 0; i < files.count; ++i) {
    //         char* filename = strrchr(files.paths[i], '/') + 1;
    //         LoadLevel(filename);
    //         LevelData* level = GetCurLevel();
    //         SaveLevel(level);
    //     }
    //     return 0;
    // }

    while (!WindowShouldClose()) {
        float delta_time = GetFrameTime();
        UIFrameStart(delta_time);
        if (IsKeyDown(KEY_LEFT_CONTROL)) {
            if (IsKeyPressed(KEY_E)) {
                EditorToggle();
            }
        }

        GameDebugEnabled = (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_D)) ^ GameDebugEnabled;
        if (GameDebugEnabled) {
            char buffer[256] = {0};
            sprintf(buffer, "FPS: %d", GetFPS());
            UIText("fps", buffer);
        }

        GameUpdate(delta_time);

        // Ensure we do this after all above updates so that the transforms reflect updated entity info.
        // TODO: Feels like this should be in GameUpdate, but run into issues when editing entities in editor mode (they
        // wont get updated in graphics). Wonder if we can control this via Flags maybe?
        for (uint32_t i = 0; i < LevelDataNumEntities(&CurLevel); ++i) {
            Entity* entity = LevelDataGetEntity(&CurLevel, i);
            EntityUpdateTransform(entity);
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        GameDraw(delta_time);

        if (IsEditorEnabled()) {
            EditorUpdate(delta_time);
        }
        UIFrameEnd();
        EndDrawing();
    }

    CloseWindow();

    return 0;
}