#ifndef UNITY_BUILD
#include "raylib.h"
#endif

static Camera3D GameCamera;

#define GetGameCamera() (GameCamera)

void SetCamera(Camera3D* camera) { GameCamera = *camera; }
