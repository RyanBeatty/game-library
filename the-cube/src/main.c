
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
//
#include "camera.c"
#endif

bool GameUpdateEnabled = true;

static void EnableGameUpdate() { GameUpdateEnabled = true; }

static void DisableGameUpdate() { GameUpdateEnabled = false; }

/////////////////////////////////////////////////////////////////////////////////////

static void GameInit() {
    InitWindow(ScreenWidth, ScreenHeight, "Game");

    SetTargetFPS(60);

    LoadLevel("level_0");

    GameCameraInit(&GameCamera);
}

static void GameUpdate(float delta_time) {
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

static void GameDraw(float delta_time) {
    BeginMode3D(GameCamera);

    Vector3 position = {0};
    for (uint32_t i = 0; i < LevelDataNumEntities(&CurLevel); ++i) {
        Entity* entity = LevelDataGetEntity(&CurLevel, i);
        DrawModel(entity->model, position, 1, WHITE);
    }

    EndMode3D();

    if (CurLevel.finished) {
        char text[256] = {0};
        sprintf(text, "Level Completed! Press Enter to Replay");
        DrawText(text, 20 + 5, 20 + 5, 18, BLACK);
    } else if (CurLevel.failed) {
        char text[256] = {0};
        sprintf(text, "Level Failed! Press Enter to Replay");
        DrawText(text, 20 + 5, 20 + 5, 18, BLACK);
    }
}

int main(void) {
    GameInit();

    while (!WindowShouldClose()) {
        float delta_time = GetFrameTime();
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
            EditorUpdate(&GameEditor, delta_time);
        }

        if (GameUpdateEnabled) {
            GameUpdate(delta_time);
        }
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
            EditorDraw(&GameEditor, delta_time);
        }
        EndDrawing();
    }

    CloseWindow();

    return 0;
}