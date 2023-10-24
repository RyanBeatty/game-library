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

#define DistanceBetweenPlayers (ScreenWidth * 0.70f)
#define HalfDistanceBetweenPlayers (DistanceBetweenPlayers / 2.0f)

static const int PaddleWidth = 20;
static const int PaddleHeight = 50;
#define HalfPaddleHeight (PaddleHeight / 2.0f)
#define PlayerDy (ScreenHeight * 0.20f)

typedef struct Player {
    Vector2 paddle_pos;

} Player;

static int BallRadius = 10;

typedef struct Ball {
    Vector2 pos;
    float speed;
} Ball;

static void DrawPlayerPaddle(Vector2 paddle_pos) {
    // TODO: Double check the displacement math here.
    DrawRectangle(paddle_pos.x, paddle_pos.y - HalfPaddleHeight, PaddleWidth, PaddleHeight, BLACK);
}

static void DrawBall(Vector2 pos) { DrawCircleV(pos, BallRadius, BLACK); }

static void UpdatePlayer(Player* player, int key_up, int key_down, float delta_time) {
    float dy = 0.0f;
    if (IsKeyDown(key_up)) {
        dy += -PlayerDy;
    }
    if (IsKeyDown(key_down)) {
        dy += PlayerDy;
    }

    player->paddle_pos.y += dy * delta_time;
    player->paddle_pos.y = player->paddle_pos.y - HalfPaddleHeight < 0 ? HalfPaddleHeight : player->paddle_pos.y;
    player->paddle_pos.y =
        player->paddle_pos.y + HalfPaddleHeight > ScreenHeight ? ScreenHeight - HalfPaddleHeight : player->paddle_pos.y;
}

static void Update(Player* player1, Player* player2, Ball* ball, float delta_time) {
    UpdatePlayer(player1, KEY_W, KEY_S, delta_time);
    UpdatePlayer(player2, KEY_UP, KEY_DOWN, delta_time);
}

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void) {
    // Initialization
    //--------------------------------------------------------------------------------------
    Player player1 = {{(ScreenWidth / 2.0f) - HalfDistanceBetweenPlayers - PaddleWidth, ScreenHeight / 2.0f}};
    Player player2 = {{(ScreenWidth / 2.0f) + HalfDistanceBetweenPlayers, ScreenHeight / 2.0f}};

    Ball ball = {{ScreenWidth / 2.0f, ScreenHeight / 2.0f}, 0.0f};

    InitWindow(ScreenWidth, ScreenHeight, "raylib [core] example - mouse input");

    SetTargetFPS(60);  // Set our game to run at 60 frames-per-second
    //---------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())  // Detect window close button or ESC key
    {
        //----------------------------------------------------------------------------------
        float delta_time = GetFrameTime();

        Update(&player1, &player2, &ball, delta_time);

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

        ClearBackground(RAYWHITE);

        // DrawText("move ball with mouse and click mouse button to change color", 10, 10, 20, DARKGRAY);
        DrawPlayerPaddle(player1.paddle_pos);
        DrawPlayerPaddle(player2.paddle_pos);
        DrawBall(ball.pos);

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow();  // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}