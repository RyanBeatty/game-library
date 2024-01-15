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

    if (!walking) {
        if (IsKeyPressed(KEY_UP)) {
            walking = true;
            T = 0;
            StartOrientation = entity->orientation;
            StartPosition = entity->position;
            EndOrientation =
                QuaternionMultiply(QuaternionFromAxisAngle((Vector3){1, 0, 0}, -90 * DEG2RAD), entity->orientation);
            entity->position = Vector3Add(entity->position, (Vector3){0, 0, -CubeSideLength});
        }
        if (IsKeyPressed(KEY_RIGHT)) {
            walking = true;
            T = 0;
            StartOrientation = entity->orientation;
            StartPosition = entity->position;
            EndOrientation =
                QuaternionMultiply(QuaternionFromAxisAngle((Vector3){0, 0, 1}, -90 * DEG2RAD), entity->orientation);
            entity->position = Vector3Add(entity->position, (Vector3){CubeSideLength, 0, 0});
        }
        if (IsKeyPressed(KEY_DOWN)) {
            walking = true;
            T = 0;
            StartOrientation = entity->orientation;
            StartPosition = entity->position;
            EndOrientation =
                QuaternionMultiply(QuaternionFromAxisAngle((Vector3){1, 0, 0}, 90 * DEG2RAD), entity->orientation);
            entity->position = Vector3Add(entity->position, (Vector3){0, 0, CubeSideLength});
        }
        if (IsKeyPressed(KEY_LEFT)) {
            walking = true;
            T = 0;
            StartOrientation = entity->orientation;
            StartPosition = entity->position;
            EndOrientation =
                QuaternionMultiply(QuaternionFromAxisAngle((Vector3){0, 0, 1}, 90 * DEG2RAD), entity->orientation);
            entity->position = Vector3Add(entity->position, (Vector3){-CubeSideLength, 0, 0});
        }
    }

    Quaternion orientation = entity->orientation;
    // Quaternion rotation = QuaternionFromMatrix(MatrixIdentity());
    Vector3 position = entity->position;
    if (walking) {
        T += delta_time;
        float t = T / 0.2;
        if (t >= 1.0f) {
            walking = false;
            entity->orientation = EndOrientation;
        } else {
            entity->orientation = QuaternionSlerp(StartOrientation, EndOrientation, t);
            position = Vector3Lerp(StartPosition, entity->position, t);
        }
    }

    if (false) {
        PrintMatrix(QuaternionToMatrix(entity->orientation));
    }

    entity->model.transform = QuaternionToMatrix(entity->orientation);
    entity->model.transform =
        MatrixMultiply(entity->model.transform, MatrixTranslate(position.x, position.y, position.z));
}

static void PlayerDraw(Entity* entity) {
    DrawModel(entity->model, (Vector3){0, 0, 0}, 1.0f, RED);
    DrawModelWires(entity->model, (Vector3){0, 0, 0}, 1.0f, GREEN);
}

/////////////////////////////////////////////////////////////////////////////////////
static Camera3D GameCamera;

void GameCameraInit(Camera3D* camera) {
    camera->position = (Vector3){0.0f, 10.0f, 10.0f};  // Camera position
    camera->target = (Vector3){0.0f, 0.0f, 0.0f};      // Camera looking at point
    camera->up = (Vector3){0.0f, 1.0f, 0.0f};          // Camera up vector (rotation towards target)
    camera->fovy = 45.0f;                              // Camera field-of-view Y
    camera->projection = CAMERA_PERSPECTIVE;           // Camera mode type
}

static void GameCameraUpdate(Camera3D* camera, Entity* entity, float delta_time) {}

/////////////////////////////////////////////////////////////////////////////////////

static void GameGridDraw() { DrawGrid(20, CubeSideLength); }

/////////////////////////////////////////////////////////////////////////////////////

static void GameInit() {
    InitWindow(ScreenWidth, ScreenHeight, "Game");

    SetTargetFPS(60);

    GameCameraInit(&GameCamera);
    PlayerInit(&Player);
}

static void GameDraw() {
    BeginDrawing();
    BeginMode3D(GameCamera);

    ClearBackground(RAYWHITE);

    PlayerDraw(&Player);
    GameGridDraw();

    EndMode3D();
    EndDrawing();
}

static void GameUpdate(float delta_time) {
    PlayerUpdate(&Player, delta_time);
    GameCameraUpdate(&GameCamera, &Player, delta_time);
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