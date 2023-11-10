/*******************************************************************************************
 *
 *   raylib [core] example - Mouse input
 *
 *   Example originally created with raylib 1.0, last time updated with raylib 4.0
 *
 *   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
 *   BSD-like license that allows static linking with closed source software
 *
 *   Copyright (c) 2014-2023 Ramon Santamaria (@raysan5)
 *
 ********************************************************************************************/

#include <stdlib.h>

#include "raylib.h"
#include "raymath.h"

static const int ScreenWidth = 800;
static const int ScreenHeight = 450;
static const int PaddleWidth = 70;
static const int PaddleHeight = 10;

static float PaddleDx = 5.0f;

typedef struct Paddle {
    Vector2 pos;
    Vector2 speed;
} Paddle;

static Rectangle PaddleGetRect(Paddle paddle) {
    return (Rectangle){.x = paddle.pos.x, .y = paddle.pos.y, .width = PaddleWidth, .height = PaddleHeight};
}

static void DrawPaddle(Paddle paddle) {
    Rectangle rect = PaddleGetRect(paddle);
    DrawRectangleRec(rect, GRAY);
};

static void Draw(Paddle paddle) {
    BeginDrawing();

    ClearBackground(RAYWHITE);

    DrawPaddle(paddle);

    EndDrawing();
}

static void UpdatePaddle(Paddle* paddle) {
    float dx = 0.0f;
    if (IsKeyDown(KEY_LEFT)) {
        dx -= PaddleDx;
    }
    if (IsKeyDown(KEY_RIGHT)) {
        dx += PaddleDx;
    }

    Vector2 new_pos = paddle->pos;
    new_pos.x += dx;

    new_pos.x = Clamp(new_pos.x, 0.0f, ScreenWidth - PaddleWidth);

    paddle->pos = new_pos;
}

static void Update(float delta_time, Paddle* paddle) { UpdatePaddle(paddle); }

int main(void) {
    Paddle paddle = {.pos = {.x = (ScreenWidth / 2) - (PaddleWidth / 2), .y = ScreenHeight * 0.8},
                     .speed = {0.0f, 0.0f}};

    InitWindow(ScreenWidth, ScreenHeight, "raylib [core] example - mouse input");

    SetTargetFPS(60);  // Set our game to run at 60 frames-per-second

    while (!WindowShouldClose())  // Detect window close button or ESC key
    {
        float delta_time = GetFrameTime();
        Update(delta_time, &paddle);
        Draw(paddle);
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow();  // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}