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

static Camera3D GameCamera;

/////////////////////////////////////////////////////////////////////////////////////
static Entity Player;

static void PlayerInit(Entity* player) {
    player->position = (Vector3){CubeSideLength / 2, CubeSideLength / 2, CubeSideLength / 2};
    player->orientation = QuaternionIdentity();
    player->model = LoadModelFromMesh(GenMeshCube(CubeSideLength, CubeSideLength, CubeSideLength));
}

static void PlayerUpdate(Entity* entity, float delta_time) {
    static bool walking = false;
    static float t_acc = 0;
    static Quaternion start_orientation;
    static Vector3 start_position;

    if (!walking) {
        if (IsKeyPressed(KEY_UP)) {
            walking = true;
            t_acc = 0;
            start_orientation = entity->orientation;
            start_position = entity->position;
            entity->orientation =
                QuaternionMultiply(QuaternionFromAxisAngle((Vector3){1, 0, 0}, -90 * DEG2RAD), entity->orientation);
            entity->position = Vector3Add(entity->position, (Vector3){0, 0, -CubeSideLength});
        }
        if (IsKeyPressed(KEY_RIGHT)) {
            walking = true;
            t_acc = 0;
            start_orientation = entity->orientation;
            start_position = entity->position;
            entity->orientation =
                QuaternionMultiply(QuaternionFromAxisAngle((Vector3){0, 0, 1}, -90 * DEG2RAD), entity->orientation);
            entity->position = Vector3Add(entity->position, (Vector3){CubeSideLength, 0, 0});
        }
        if (IsKeyPressed(KEY_DOWN)) {
            walking = true;
            t_acc = 0;
            start_orientation = entity->orientation;
            start_position = entity->position;
            entity->orientation =
                QuaternionMultiply(QuaternionFromAxisAngle((Vector3){1, 0, 0}, 90 * DEG2RAD), entity->orientation);
            entity->position = Vector3Add(entity->position, (Vector3){0, 0, CubeSideLength});
        }
        if (IsKeyPressed(KEY_LEFT)) {
            walking = true;
            t_acc = 0;
            start_orientation = entity->orientation;
            start_position = entity->position;
            entity->orientation =
                QuaternionMultiply(QuaternionFromAxisAngle((Vector3){0, 0, 1}, 90 * DEG2RAD), entity->orientation);
            entity->position = Vector3Add(entity->position, (Vector3){-CubeSideLength, 0, 0});
        }
    }

    Quaternion orientation = entity->orientation;
    Quaternion rotation = QuaternionFromMatrix(MatrixIdentity());
    Vector3 position = {0, 0, 0};
    if (walking) {
        t_acc += delta_time;
        float t = t_acc / 0.2;
        if (t >= 1.0f) {
            walking = false;
        } else {
            printf("t: %f\n", t);
            rotation = QuaternionSlerp(start_orientation, orientation, t);
            position = Vector3Lerp(start_position, entity->position, t);
        }
    }

    rotation = QuaternionMultiply(rotation, orientation);
    position = Vector3Subtract(entity->position, position);
    if (Vector3Equals(position, entity->position)) {
        position = (Vector3){0, 0, 0};
    }
    breakpoint();
    entity->model.transform = QuaternionToMatrix(rotation);
    entity->model.transform =
        MatrixMultiply(entity->model.transform, MatrixTranslate(-position.x, -position.y, -position.z));
}

static void PlayerDraw(Entity* entity) {
    DrawModel(entity->model, entity->position, 1.0f, RED);
    DrawModelWires(entity->model, entity->position, 1.0f, GREEN);
}

/////////////////////////////////////////////////////////////////////////////////////

static void GameGridDraw() { DrawGrid(20, CubeSideLength); }

/////////////////////////////////////////////////////////////////////////////////////

static void GameInit() {
    InitWindow(ScreenWidth, ScreenHeight, "Game");

    SetTargetFPS(60);

    GameCamera.position = (Vector3){0.0f, 10.0f, 10.0f};  // Camera position
    GameCamera.target = (Vector3){0.0f, 0.0f, 0.0f};      // Camera looking at point
    GameCamera.up = (Vector3){0.0f, 1.0f, 0.0f};          // Camera up vector (rotation towards target)
    GameCamera.fovy = 45.0f;                              // Camera field-of-view Y
    GameCamera.projection = CAMERA_PERSPECTIVE;           // Camera mode type

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

static void GameUpdate(float delta_time) { PlayerUpdate(&Player, delta_time); }

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