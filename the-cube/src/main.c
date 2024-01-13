#include <stdlib.h>

#include "raylib.h"
#include "raymath.h"

const int ScreenWidth = 800;
const int ScreenHeight = 450;

static const float CubeSideLength = 2.0f;

typedef struct Entity {
    Vector3 position;
} Entity;

static Camera3D GameCamera;

/////////////////////////////////////////////////////////////////////////////////////
static Entity Player;

static void PlayerInit(Entity* player) {
    player->position = (Vector3){CubeSideLength / 2, CubeSideLength / 2, CubeSideLength / 2};
}

static void PlayerUpdate(Entity* entity, float delta_time) {
    Vector3 speed = {};
    if (IsKeyPressed(KEY_UP)) {
        speed.z -= 1.0f;
    }
    if (IsKeyPressed(KEY_RIGHT)) {
        speed.x += 1.0f;
    }
    if (IsKeyPressed(KEY_DOWN)) {
        speed.z += 1.0f;
    }
    if (IsKeyPressed(KEY_LEFT)) {
        speed.x -= 1.0f;
    }

    speed = Vector3Normalize(speed);
    speed = Vector3Scale(speed, CubeSideLength);

    entity->position = Vector3Add(entity->position, speed);
}

static void PlayerDraw(Entity* entity) {
    DrawCube(entity->position, CubeSideLength, CubeSideLength, CubeSideLength, RED);
    DrawCubeWires(entity->position, CubeSideLength, CubeSideLength, CubeSideLength, BLUE);
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