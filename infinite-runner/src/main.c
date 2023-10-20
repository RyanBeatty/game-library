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

const int ScreenWidth = 800 * 2;
const int ScreenHeight = 450 * 2;

const int BackgrounHeight = 100;

const int PlayerRectWidth = 60;
const int PlayerRectHeight = 60;
#define HalfPlayerRectWidth (PlayerRectWidth / 2)
#define HalfPlayerRectHeight (PlayerRectHeight / 2)
#define PlayerOriginX (ScreenWidth / 2)
#define PlayerOriginY (ScreenHeight - BackgrounHeight - HalfPlayerRectHeight)
const float PlayerSpeedX = 300.0f;
const float PlayerSpeedY = 300.0f;
const float Gravity = 400.0f;

typedef struct Player {
    Vector2 position;
    int dy;
    bool can_jump;
} Player;

static void DrawObstacles(Rectangle* obstacles) {
    for (size_t i = 0; i < arrlen(obstacles); ++i) {
        DrawRectangleRec(obstacles[i], GRAY);
    }
}

static Rectangle GetPlayerRec(Vector2 position) {
    Rectangle rec = {position.x - HalfPlayerRectWidth, position.y - HalfPlayerRectHeight, PlayerRectWidth,
                     PlayerRectHeight};
    return rec;
}
static void DrawPlayer(Player player) {
    Rectangle rec = GetPlayerRec(player.position);
    DrawRectangleRec(rec, RED);
}
static void UpdatePlayer(Player* player, float delta_time, Rectangle* obstacles) {
    float dx = 0.0f;
    if (IsKeyDown(KEY_LEFT)) {
        dx = -PlayerSpeedX;
    } else if (IsKeyDown(KEY_RIGHT)) {
        dx = PlayerSpeedX;
    }
    dx *= delta_time;

    if (IsKeyDown(KEY_SPACE) && player->can_jump) {
        player->can_jump = false;
        player->dy = -PlayerSpeedY;
    }

    float dy = player->dy * delta_time;

    Vector2 new_pos = {player->position.x + dx, player->position.y + dy};
    Rectangle new_player_rec = GetPlayerRec(new_pos);
    for (size_t i = 0; i < arrlen(obstacles); ++i) {
        Rectangle obstacle = obstacles[i];
        if (CheckCollisionRecs(new_player_rec, obstacle)) {
            if (player->position.x - HalfPlayerRectWidth >= obstacle.x &&
                player->position.x - HalfPlayerRectWidth + dx <= obstacle.x) {
                player->position.x = obstacle.x + HalfPlayerRectWidth;
                dx = 0.0f;
            } else if (player->position.x + HalfPlayerRectWidth <= obstacle.x &&
                       player->position.x + HalfPlayerRectWidth + dx >= obstacle.x) {
                // TODO: Might need to factor in obstacle rect width.
                player->position.x = obstacle.x - HalfPlayerRectWidth;
                dx = 0.0f;
            }

            if (player->position.y - HalfPlayerRectHeight >= obstacle.y + obstacle.height &&
                player->position.y - HalfPlayerRectHeight + dy <= obstacle.y + obstacle.height) {
                player->position.y = obstacle.y + obstacle.height + HalfPlayerRectHeight;
                dy = 0.0f;
            } else if (player->position.y + HalfPlayerRectHeight <= obstacle.y &&
                       player->position.y + HalfPlayerRectHeight + dy >= obstacle.y) {
                player->position.y = obstacle.y - HalfPlayerRectHeight;
                player->can_jump = true;
                dy = 0.0f;
            }
        }
    }

    player->position.x += dx;
    player->position.y += dy;
    player->dy += Gravity * delta_time;
}

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void) {
    // Initialization
    //--------------------------------------------------------------------------------------

    Player player = {{PlayerOriginX, PlayerOriginY}, 0, true};
    Rectangle* obstacles = NULL;

    // Environment.
    Rectangle background = {0, ScreenHeight - BackgrounHeight, ScreenWidth, BackgrounHeight};
    Rectangle left_wall = {0, 0, 1, ScreenHeight};
    Rectangle right_wall = {ScreenWidth, 0, 1, ScreenHeight};
    Rectangle test_block = {ScreenWidth / 2, ScreenHeight * 0.75, ScreenWidth, 50};
    arrpush(obstacles, background);
    arrpush(obstacles, left_wall);
    arrpush(obstacles, right_wall);
    arrpush(obstacles, test_block);

    // SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(ScreenWidth, ScreenHeight, "raylib [core] example - mouse input");

    SetTargetFPS(60);  // Set our game to run at 60 frames-per-second
    //---------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())  // Detect window close button or ESC key
    {
        //----------------------------------------------------------------------------------
        float delta_time = GetFrameTime();

        UpdatePlayer(&player, delta_time, obstacles);

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

        ClearBackground(RAYWHITE);

        DrawObstacles(obstacles);
        DrawPlayer(player);

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow();  // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}