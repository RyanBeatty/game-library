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

static void GameCameraFollowEntity(Camera3D* camera, Entity* player) {
    camera->position = (Vector3){player->position.x, 10.0f, player->position.z + 10.0f};
    camera->target = player->position;
}

void GameCameraInit(Camera3D* camera, Entity* player) {
    camera->position = (Vector3){0.0f, 10.0f, 10.0f};  // Camera position
    camera->target = (Vector3){0.0f, 0.0f, 0.0f};      // Camera looking at point
    GameCameraFollowEntity(camera, player);
    camera->up = (Vector3){0.0f, 1.0f, 0.0f};  // Camera up vector (rotation towards target)
    camera->fovy = 45.0f;                      // Camera field-of-view Y
    camera->projection = CAMERA_PERSPECTIVE;   // Camera mode type
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
            entity->model.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = GRAY;
        }
    }
}
/////////////////////////////////////////////////////////////////////////////////////
typedef enum EditorState {
    ES_DISABLED = 0,
    ES_IDLE,
    ES_EDITING,

    ES_COUNT
} EditorState;

typedef struct Editor {
    EditorState editor_state;
    Entity* focus;
} Editor;

static Editor GameEditor = {0};

static void EditorUpdate(Editor* editor) {
    if (IsKeyPressed(KEY_E)) {
        editor->editor_state = editor->editor_state == ES_DISABLED ? ES_IDLE : ES_DISABLED;
    }

    switch (editor->editor_state) {
        case ES_DISABLED: {
            break;
        }
        case ES_IDLE: {
            if (IsKeyPressed(KEY_A)) {
                editor->focus = CreateEntity();
                editor->focus->model = LoadModelFromMesh(GenMeshCube(CubeSideLength, CubeSideLength, CubeSideLength));
                editor->focus->model.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = BLACK;
                editor->editor_state = ES_EDITING;
            }
            break;
        }
        case ES_EDITING: {
            // TODO: Need to setup transform matrices.
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
            }
            break;
        }
        default: {
            exit(EXIT_FAILURE);
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////
static bool DebugMode = false;

static void DebugModeEnable() { UpdateCamera(&GameCamera, CAMERA_FREE); }

static void DebugModeDisable() { UpdateCamera(&GameCamera, CAMERA_PERSPECTIVE); }

static void DebugModeUpdate(float delta_time) {
    static const float RotationSpeed = 90 * DEG2RAD;

    Matrix rotation = MatrixIdentity();
    if (IsKeyDown(KEY_LEFT)) {
        rotation = MatrixRotateY(-RotationSpeed * delta_time);
    } else if (IsKeyDown(KEY_RIGHT)) {
        rotation = MatrixRotateY(RotationSpeed * delta_time);
    }

    // TODO: For some reason this seems to not be a perfect circular rotation about the target.
    // When we rotate behind the object, the perspective seems to shift. Maybe related to field of view??
    GameCamera.position = Vector3Transform(GameCamera.position, rotation);
}

static void DrawDebugMode() {
    // Not in Mode3D. All coords are screen space.
    if (DebugMode) {
        DrawFPS(10, 10);
    }
}

/////////////////////////////////////////////////////////////////////////////////////

static void GameInit() {
    InitWindow(ScreenWidth, ScreenHeight, "Game");

    SetTargetFPS(60);

    Entity* player = CreateEntity();
    PlayerInit(player);

    RegisterBackgroundEntities();

    GameCameraInit(&GameCamera, player);
}

static void GameUpdate(float delta_time) {
    if (IsKeyPressed(KEY_D)) {
        DebugMode = !DebugMode;
        if (DebugMode) {
            DebugModeEnable();
        } else {
            DebugModeDisable();
        }
    }

    if (!DebugMode) {
        Entity* player = &GameEntities[0];
        PlayerUpdate(player, delta_time);
        GameCameraFollowEntity(&GameCamera, player);
    } else {
        DebugModeUpdate(delta_time);
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

    for (uint32_t i = 0; i < arrlen(GameEntities); ++i) {
        DrawModel(GameEntities[i].model, (Vector3){0}, 1, WHITE);
    }

    EndMode3D();
    DrawDebugMode();

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