#ifndef UNITY_BUILD
#include "raylib.h"
#endif

static Camera3D GameCamera;

void GameCameraInit(Camera3D* camera) {
    camera->position = (Vector3){0.0f, 20.0f, 20.0f};
    camera->target = (Vector3){0.0f, 0.0f, 0.0f};
    camera->up = (Vector3){0.0f, 1.0f, 0.0f};
    camera->fovy = 45.0f;
    camera->projection = CAMERA_PERSPECTIVE;
}