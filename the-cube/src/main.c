#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

const int ScreenWidth = 800;
const int ScreenHeight = 450;

static const float CubeSideLength = 2.0f;

static void PrintMatrix(Matrix m) {
    float16 mv = MatrixToFloatV(m);
    for (int i = 0; i < 16; ++i) {
        if (i % 4 == 0) {
            printf("\n");
        }
        printf("%f ", mv.v[i]);
    }
    printf("\n");
}

typedef struct Entity {
    uint32_t idx;
    Vector3 position;
    Quaternion orientation;

    Model model;
} Entity;

Entity* GameEntities = NULL;

static Entity* CreateEntity() {
    Entity* entity = arraddnptr(GameEntities, 1);
    entity->idx = arrlen(GameEntities) - 1;
    return entity;
}

static void EntityUpdateTransform(Entity* entity) {
    // Remember matrices are applied left to right order, so the rotation is first.
    entity->model.transform =
        MatrixMultiply(QuaternionToMatrix(entity->orientation),
                       MatrixTranslate(entity->position.x, entity->position.y, entity->position.z));
}

/////////////////////////////////////////////////////////////////////////////////////
static void PlayerInit(Entity* player) {
    player->position = (Vector3){CubeSideLength / 2, CubeSideLength / 2, CubeSideLength / 2};
    player->orientation = QuaternionIdentity();
    player->model = LoadModelFromMesh(GenMeshCube(CubeSideLength, CubeSideLength, CubeSideLength));
    player->model.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = RED;
}

static void PlayerUpdate(Entity* entity, float delta_time) {
    static bool walking = false;
    static float T = 0;
    static Quaternion StartOrientation;
    static Quaternion EndOrientation;
    static Vector3 StartPosition;
    static Vector3 EndPosition;

    if (!walking) {
        if (IsKeyPressed(KEY_UP)) {
            walking = true;
            T = 0;
            StartOrientation = entity->orientation;
            StartPosition = entity->position;
            EndOrientation =
                QuaternionMultiply(QuaternionFromAxisAngle((Vector3){1, 0, 0}, -90 * DEG2RAD), entity->orientation);
            EndPosition = Vector3Add(entity->position, (Vector3){0, 0, -CubeSideLength});
        }
        if (IsKeyPressed(KEY_RIGHT)) {
            walking = true;
            T = 0;
            StartOrientation = entity->orientation;
            StartPosition = entity->position;
            EndOrientation =
                QuaternionMultiply(QuaternionFromAxisAngle((Vector3){0, 0, 1}, -90 * DEG2RAD), entity->orientation);
            EndPosition = Vector3Add(entity->position, (Vector3){CubeSideLength, 0, 0});
        }
        if (IsKeyPressed(KEY_DOWN)) {
            walking = true;
            T = 0;
            StartOrientation = entity->orientation;
            StartPosition = entity->position;
            EndOrientation =
                QuaternionMultiply(QuaternionFromAxisAngle((Vector3){1, 0, 0}, 90 * DEG2RAD), entity->orientation);
            EndPosition = Vector3Add(entity->position, (Vector3){0, 0, CubeSideLength});
        }
        if (IsKeyPressed(KEY_LEFT)) {
            walking = true;
            T = 0;
            StartOrientation = entity->orientation;
            StartPosition = entity->position;
            EndOrientation =
                QuaternionMultiply(QuaternionFromAxisAngle((Vector3){0, 0, 1}, 90 * DEG2RAD), entity->orientation);
            EndPosition = Vector3Add(entity->position, (Vector3){-CubeSideLength, 0, 0});
        }
    }

    if (walking) {
        T += delta_time;
        float t = T / 0.2;
        if (t >= 1.0f) {
            walking = false;
            entity->orientation = EndOrientation;
            entity->position = EndPosition;
        } else {
            entity->orientation = QuaternionSlerp(StartOrientation, EndOrientation, t);
            entity->position = Vector3Lerp(StartPosition, EndPosition, t);
        }
    }

    if (false) {
        PrintMatrix(QuaternionToMatrix(entity->orientation));
    }
}

/////////////////////////////////////////////////////////////////////////////////////
static Camera3D GameCamera;

void GameCameraInit(Camera3D* camera) {
    camera->position = (Vector3){0.0f, 10.0f, 20.0f};
    camera->target = (Vector3){0.0f, 0.0f, 0.0f};
    camera->up = (Vector3){0.0f, 1.0f, 0.0f};
    camera->fovy = 45.0f;
    camera->projection = CAMERA_PERSPECTIVE;
}

/////////////////////////////////////////////////////////////////////////////////////

#define GameGridNumRows (10)
#define GameGridNumCols (10)
#define GameGridNumSpaces ((GameGridNumRows) * (GameGridNumCols))

static void RegisterBackgroundEntities() {
    float length = CubeSideLength;
    float half_length = length / 2.0f;
    for (int i = 0, r = 0; r < GameGridNumRows; ++r) {
        for (int c = 0; c < GameGridNumCols; ++c, ++i) {
            Entity* entity = CreateEntity();
            entity->model = LoadModelFromMesh(GenMeshCube(CubeSideLength, CubeSideLength, CubeSideLength));
            entity->position = (Vector3){(r * length) + half_length, -half_length, (c * length) + half_length};
            // Hack shift position so that camera get capture full scene.
            entity->position = Vector3Add(entity->position, (Vector3){-10.0f, 0, -10.0f});
            entity->model.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = GRAY;
        }
    }
}
/////////////////////////////////////////////////////////////////////////////////////
typedef enum EditorState {
    ES_IDLE = 0,
    ES_EDITING,

    ES_COUNT
} EditorState;

typedef struct Editor {
    EditorState editor_state;
    Entity* focus;
} Editor;

static Editor GameEditor = {0};
// Yuck -- figure out something better later.
static bool EditorEnabled = false;

static void EditorEnable() {
    EditorEnabled = true;
    GameEditor = (Editor){0};
}

static void EditorDisable() {
    EditorEnabled = false;
    GameEditor = (Editor){0};
}

static void EditorUpdate(Editor* editor) {
    assert(EditorEnabled);

    switch (editor->editor_state) {
        case ES_IDLE: {
            if (IsKeyPressed(KEY_A)) {
                Entity* focus = CreateEntity();
                focus->model = LoadModelFromMesh(GenMeshCube(CubeSideLength, CubeSideLength, CubeSideLength));
                focus->position = (Vector3){CubeSideLength / 2, CubeSideLength / 2, CubeSideLength / 2};
                focus->model.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = BLACK;
                editor->focus = focus;
                editor->editor_state = ES_EDITING;
            }
            break;
        }
        case ES_EDITING: {
            if (IsKeyPressed(KEY_UP)) {
                editor->focus->position.z -= CubeSideLength;
            } else if (IsKeyPressed(KEY_RIGHT)) {
                editor->focus->position.x += CubeSideLength;
            } else if (IsKeyPressed(KEY_DOWN)) {
                editor->focus->position.z += CubeSideLength;
            } else if (IsKeyPressed(KEY_LEFT)) {
                editor->focus->position.x -= CubeSideLength;
            } else if (IsKeyPressed(KEY_PAGE_UP)) {
                editor->focus->position.y += CubeSideLength;
            } else if (IsKeyPressed(KEY_PAGE_DOWN)) {
                editor->focus->position.y -= CubeSideLength;
            } else if (IsKeyPressed(KEY_ENTER)) {
                editor->focus = NULL;
                editor->editor_state = ES_IDLE;
            }
            break;
        }
        default: {
            exit(EXIT_FAILURE);
        }
    }
}

static void EditorDraw(Editor* editor) {
    if (EditorEnabled) {
        // Not in Mode3D. All coords are screen space.
        DrawFPS(10, 10);
        DrawText("Editor Enabled", 100, 10, 20, BLACK);
    }
}

/////////////////////////////////////////////////////////////////////////////////////

static void GameInit() {
    InitWindow(ScreenWidth, ScreenHeight, "Game");

    SetTargetFPS(60);

    Entity* player = CreateEntity();
    PlayerInit(player);

    RegisterBackgroundEntities();

    GameCameraInit(&GameCamera);
}

static void GameUpdate(float delta_time) {
    if (IsKeyPressed(KEY_E)) {
        if (!EditorEnabled) {
            EditorEnable();
        } else {
            EditorDisable();
        }
    }

    if (!EditorEnabled) {
        Entity* player = &GameEntities[0];
        PlayerUpdate(player, delta_time);
    } else {
        EditorUpdate(&GameEditor);
    }

    // Ensure we do this after all above updates so that the transforms reflect updated entity info.
    for (uint32_t i = 0; i < arrlen(GameEntities); ++i) {
        EntityUpdateTransform(&GameEntities[i]);
    }
}

static void GameDraw() {
    ClearBackground(RAYWHITE);

    BeginDrawing();
    BeginMode3D(GameCamera);

    Vector3 position = {0};
    for (uint32_t i = 0; i < arrlen(GameEntities); ++i) {
        DrawModel(GameEntities[i].model, position, 1, WHITE);
    }

    EndMode3D();
    EditorDraw(&GameEditor);

    EndDrawing();
}

int main(void) {
    GameInit();

    while (!WindowShouldClose()) {
        float delta_time = GetFrameTime();
        GameUpdate(delta_time);
        GameDraw();
    }

    CloseWindow();

    return 0;
}