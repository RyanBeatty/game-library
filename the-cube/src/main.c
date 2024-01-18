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
    Vector3 position;
    Quaternion orientation;

    Model model;
} Entity;

/////////////////////////////////////////////////////////////////////////////////////
static Entity Player;

static void PlayerInit(Entity* player) {
    player->position = (Vector3){CubeSideLength / 2, CubeSideLength / 2, CubeSideLength / 2};
    player->orientation = QuaternionIdentity();
    player->model = LoadModelFromMesh(GenMeshCube(CubeSideLength, CubeSideLength, CubeSideLength));
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

    entity->model.transform = QuaternionToMatrix(entity->orientation);
    entity->model.transform = MatrixMultiply(
        entity->model.transform, MatrixTranslate(entity->position.x, entity->position.y, entity->position.z));
}

static void PlayerDraw(Entity* entity) {
    DrawModel(entity->model, (Vector3){0, 0, 0}, 1.0f, RED);
    DrawModelWires(entity->model, (Vector3){0, 0, 0}, 1.0f, GREEN);
}

/////////////////////////////////////////////////////////////////////////////////////
static Camera3D GameCamera;

static void GameCameraUpdate(Camera3D* camera, Entity* entity) {
    camera->position = (Vector3){entity->position.x, 10.0f, entity->position.z + 10.0f};
    camera->target = entity->position;
}

void GameCameraInit(Camera3D* camera, Entity* player) {
    camera->position = (Vector3){0.0f, 10.0f, 10.0f};  // Camera position
    camera->target = (Vector3){0.0f, 0.0f, 0.0f};      // Camera looking at point
    GameCameraUpdate(camera, player);
    camera->up = (Vector3){0.0f, 1.0f, 0.0f};  // Camera up vector (rotation towards target)
    camera->fovy = 45.0f;                      // Camera field-of-view Y
    camera->projection = CAMERA_PERSPECTIVE;   // Camera mode type
}

/////////////////////////////////////////////////////////////////////////////////////

static Entity GameGrid;
static Material GameGridMaterial;
#define GameGridNumRows (100)
#define GameGridNumCols (100)
#define GameGridNumSpaces ((GameGridNumRows) * (GameGridNumCols))
static Matrix GameGridTransforms[GameGridNumRows * GameGridNumCols];

static void GameGridInit(Entity* game_grid) {
    GameGridMaterial = LoadMaterialDefault();
    Shader shader =
        LoadShader("resources/shaders/game_grid_instancing.vs", "resources/shaders/game_grid_instancing.fs");
    assert(IsShaderReady(shader));
    // Set raylib default locations.
    shader.locs[SHADER_LOC_MATRIX_MVP] = GetShaderLocation(shader, "mvp");
    shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(shader, "viewPos");
    shader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocationAttrib(shader, "instanceTransform");
    GameGridMaterial.shader = shader;
    GameGridMaterial.maps[MATERIAL_MAP_DIFFUSE].color = BLUE;

    float length = CubeSideLength;
    float half_length = length / 2.0f;
    for (int i = 0, r = 0; r < GameGridNumRows; ++r) {
        for (int c = 0; c < GameGridNumCols; ++c, ++i) {
            GameGridTransforms[i] = MatrixTranslate((r * length) + half_length, -length, (c * length) + half_length);
        }
    }
    game_grid->model = LoadModelFromMesh(GenMeshCube(CubeSideLength, CubeSideLength, CubeSideLength));
}

static void GameGridDraw(Entity* game_grid) {
    assert(game_grid->model.meshCount == 1);
    DrawMeshInstanced(game_grid->model.meshes[0], GameGridMaterial, GameGridTransforms, GameGridNumSpaces);
}

/////////////////////////////////////////////////////////////////////////////////////
static bool DebugMode = false;

void DrawDebug() {
    // Not in Mode3D. All coords are screen space.
    if (DebugMode) {
        DrawFPS(10, 10);
    }
}

/////////////////////////////////////////////////////////////////////////////////////

static void GameInit() {
    InitWindow(ScreenWidth, ScreenHeight, "Game");

    SetTargetFPS(60);

    PlayerInit(&Player);
    GameCameraInit(&GameCamera, &Player);
    GameGridInit(&GameGrid);
}

static void GameUpdate(float delta_time) {
    PlayerUpdate(&Player, delta_time);
    GameCameraUpdate(&GameCamera, &Player);

    if (IsKeyPressed(KEY_D)) {
        DebugMode = !DebugMode;
    }
}

static void GameDraw() {
    BeginDrawing();
    BeginMode3D(GameCamera);

    ClearBackground(RAYWHITE);

    PlayerDraw(&Player);
    GameGridDraw(&GameGrid);

    EndMode3D();
    DrawDebug();

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