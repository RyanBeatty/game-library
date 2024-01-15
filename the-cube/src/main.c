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

// static Matrix PlayerRotateTransform(Matrix rotation, float delta_time) {
//     Matrix target = {1, 0, 0, 0,
//                         0, 0, 1, 0,
//                         0, -1, 0, 0};
//     return (Matrix){Vector3Lerp(rotation.)}

// }

static void PlayerUpdate(Entity* entity, float delta_time) {
    static bool walking = false;
    static float angle = 0;
    static float t_acc = 0;
    static Quaternion start_orientation;

    if (IsKeyPressed(KEY_UP)) {
        walking = true;
        t_acc = 0;
        start_orientation = entity->orientation;
        Matrix target = (Matrix){
            0,  1.0, 0, 0,  //
            -1, 0,   0, 0,  //
            0,  0,   1, 0,  //
            0,  0,   0, 1,
        };
        entity->orientation = QuaternionMultiply(QuaternionFromMatrix(target), entity->orientation);
        // Matrix rotation = MatrixRotateZ(angle * RAD2DEG);
        // entity->model.transform = MatrixMultiply(entity->model.transform, rotation);
    }

    Quaternion orientation = entity->orientation;
    Quaternion rotation = QuaternionFromMatrix(MatrixIdentity());
    if (walking) {
        t_acc += delta_time;
        float t = t_acc / 0.2;
        if (t >= 1.0f) {
            walking = false;
        } else {
            printf("t: %f\n", t);
            rotation = QuaternionSlerp(start_orientation, orientation, t);
        }
    }

    printf("orientation:");
    PrintMatrix(QuaternionToMatrix(orientation));
    printf("rotation:");
    PrintMatrix(QuaternionToMatrix(rotation));

    orientation = QuaternionMultiply(rotation, orientation);
    entity->model.transform = QuaternionToMatrix(orientation);

    //         printf("foo\n");
    //         // Matrix target = {1, 0, 0, 0, 0, 0, 1, 0, 0, -1, 0, 0};
    //         Matrix target = {0, -1, 0, 0,  //
    //                          1, 0,  0, 0,  //
    //                          0, 0,  1, 0};
    //         entity->rotation.m0 += 0.2f * (target.m0 - entity->rotation.m0);
    //         entity->rotation.m1 += 0.2f * (target.m1 - entity->rotation.m1);
    //         entity->rotation.m2 += 0.2f * (target.m2 - entity->rotation.m2);
    //         entity->rotation.m4 += 0.2f * (target.m4 - entity->rotation.m4);
    //         entity->rotation.m5 += 0.2f * (target.m5 - entity->rotation.m5);
    //         entity->rotation.m6 += 0.2f * (target.m6 - entity->rotation.m6);
    //         entity->rotation.m8 += 0.2f * (target.m8 - entity->rotation.m8);
    //         entity->rotation.m9 += 0.2f * (target.m9 - entity->rotation.m9);
    //         entity->rotation.m10 += 0.2f * (target.m10 - entity->rotation.m10);
    //         entity->rotation.m12 += 0.2f * (target.m12 - entity->rotation.m12);
    //         entity->rotation.m13 += 0.2f * (target.m13 - entity->rotation.m13);
    //         entity->rotation.m14 += 0.2f * (target.m14 - entity->rotation.m14);
    //     }
    // }

    // printf("================================\n");
    // printf("%f ", entity->rotation.m0);
    // printf("%f ", entity->rotation.m4);
    // printf("%f ", entity->rotation.m8);
    // printf("%f ", entity->rotation.m12);
    // printf("\n");
    // printf("%f ", entity->rotation.m1);
    // printf("%f ", entity->rotation.m5);
    // printf("%f ", entity->rotation.m9);
    // printf("%f ", entity->rotation.m13);
    // printf("\n");
    // printf("%f ", entity->rotation.m2);
    // printf("%f ", entity->rotation.m6);
    // printf("%f ", entity->rotation.m10);
    // printf("%f ", entity->rotation.m14);
    // printf("\n");
    // printf("%f ", entity->rotation.m3);
    // printf("%f ", entity->rotation.m7);
    // printf("%f ", entity->rotation.m11);
    // printf("%f ", entity->rotation.m15);
    // printf("===========================\n");
    // Vector3 speed = {};
    // if (IsKeyPressed(KEY_UP)) {
    //     speed.z -= 1.0f;
    // }
    // if (IsKeyPressed(KEY_RIGHT)) {
    //     speed.x += 1.0f;
    // }
    // if (IsKeyPressed(KEY_DOWN)) {
    //     speed.z += 1.0f;
    // }
    // if (IsKeyPressed(KEY_LEFT)) {
    //     speed.x -= 1.0f;
    // }

    // speed = Vector3Normalize(speed);
    // speed = Vector3Scale(speed, CubeSideLength);

    // entity->position = Vector3Add(entity->position, speed);
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