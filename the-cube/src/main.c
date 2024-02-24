
#ifndef UNITY_BUILD
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "entity.c"
#include "level.c"
#include "npc.c"
#include "player.c"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "serde.c"
//
#include "editor.c"
#endif

/////////////////////////////////////////////////////////////////////////////////////
static Camera3D GameCamera;

void GameCameraInit(Camera3D* camera) {
    camera->position = (Vector3){0.0f, 20.0f, 20.0f};
    camera->target = (Vector3){0.0f, 0.0f, 0.0f};
    camera->up = (Vector3){0.0f, 1.0f, 0.0f};
    camera->fovy = 45.0f;
    camera->projection = CAMERA_PERSPECTIVE;
}

/////////////////////////////////////////////////////////////////////////////////////

static void GameInit() {
    InitWindow(ScreenWidth, ScreenHeight, "Game");

    SetTargetFPS(60);

    LoadLevel("level_0");

    GameCameraInit(&GameCamera);
}

static void GameUpdate(float delta_time) {
    if (IsKeyDown(KEY_LEFT_CONTROL)) {
        if (IsKeyPressed(KEY_E)) {
            if (!IsEditorEnabled()) {
                EditorEnable();
            } else {
                EditorDisable();
            }
        }
    }

    if (IsEditorEnabled()) {
        EditorUpdate(&GameEditor);
    }

    if (!IsEditorEnabled()) {
        if (CurLevel.finished) {
            if (IsKeyPressed(KEY_ENTER)) {
                LoadLevel(CurLevel.name);
            }
        } else if (CurLevel.failed) {
            if (IsKeyPressed(KEY_ENTER)) {
                LoadLevel(CurLevel.name);
            }
        } else {
            for (uint32_t i = 0; i < LevelDataNumEntities(&CurLevel); ++i) {
                Entity* entity = LevelDataGetEntity(&CurLevel, i);
                EntityUpdate(entity, delta_time);
            }
        }
    }

    // Ensure we do this after all above updates so that the transforms reflect updated entity info.
    for (uint32_t i = 0; i < LevelDataNumEntities(&CurLevel); ++i) {
        Entity* entity = LevelDataGetEntity(&CurLevel, i);
        EntityUpdateTransform(entity);
    }
}

static void GameDraw(float delta_time) {
    ClearBackground(RAYWHITE);

    BeginDrawing();

    BeginMode3D(GameCamera);

    Vector3 position = {0};
    for (uint32_t i = 0; i < LevelDataNumEntities(&CurLevel); ++i) {
        Entity* entity = LevelDataGetEntity(&CurLevel, i);
        DrawModel(entity->model, position, 1, WHITE);
    }

    EndMode3D();
    EditorDraw(&GameEditor, delta_time);

    if (CurLevel.finished) {
        char text[256] = {0};
        sprintf(text, "Level Completed! Press Enter to Replay");
        DrawText(text, 20 + 5, 20 + 5, 18, BLACK);
    } else if (CurLevel.failed) {
        char text[256] = {0};
        sprintf(text, "Level Failed! Press Enter to Replay");
        DrawText(text, 20 + 5, 20 + 5, 18, BLACK);
    }

    EndDrawing();
}

int main(void) {
    GameInit();

    while (!WindowShouldClose()) {
        float delta_time = GetFrameTime();
        GameUpdate(delta_time);
        GameDraw(delta_time);
    }

    CloseWindow();

    return 0;
}