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

static const float BallRadius = 5.0f;

typedef struct Ball {
    Vector2 pos;
    Vector2 speed;
    bool is_served;
} Ball;

static void DrawBall(Ball ball) { DrawCircleV(ball.pos, BallRadius, BLACK); }

static void Draw(Paddle paddle, Ball ball) {
    BeginDrawing();

    ClearBackground(RAYWHITE);

    DrawPaddle(paddle);
    DrawBall(ball);

    EndDrawing();
}

static void UpdateBall(Ball* ball, Vector2 paddle_pos, float delta_time) {
    Ball new_ball = *ball;
    if (!ball->is_served) {
        new_ball.pos = paddle_pos;
        new_ball.pos.x += (PaddleWidth / 2.0f);
        new_ball.pos.y -= BallRadius;
        new_ball.speed = (Vector2){.x = 0.0f, .y = 0.0f};

        if (IsKeyDown(KEY_SPACE)) {
            new_ball.is_served = true;
            new_ball.speed = (Vector2){.x = 0.0f, .y = -300.0f};
        }
    }

    new_ball.pos = Vector2Add(new_ball.pos, Vector2Scale(new_ball.speed, delta_time));

    *ball = new_ball;
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

static void Update(float delta_time, Paddle* paddle, Ball* ball) {
    UpdatePaddle(paddle);
    UpdateBall(ball, paddle->pos, delta_time);
}

int main(void) {
    Paddle paddle = {.pos = {.x = (ScreenWidth / 2) - (PaddleWidth / 2), .y = ScreenHeight * 0.8},
                     .speed = {0.0f, 0.0f}};
    Ball ball = {.pos = {100, 10}, {0, 0}, false};

    InitWindow(ScreenWidth, ScreenHeight, "raylib [core] example - mouse input");

    SetTargetFPS(60);  // Set our game to run at 60 frames-per-second

    while (!WindowShouldClose())  // Detect window close button or ESC key
    {
        float delta_time = GetFrameTime();
        Update(delta_time, &paddle, &ball);
        Draw(paddle, ball);
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow();  // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}