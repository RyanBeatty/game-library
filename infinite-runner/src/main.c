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

static void DrawBackground(int screen_width, int screen_height) {
    DrawRectangle(0, screen_height - BackgrounHeight, screen_width, BackgrounHeight, GRAY);
}
static void DrawPlayer(Player player) {
    DrawRectangle(player.position.x - HalfPlayerRectWidth, player.position.y - HalfPlayerRectHeight, PlayerRectWidth,
                  PlayerRectHeight, RED);
}
static void UpdatePlayer(Player* player, float delta_time) {
    float dx = 0.0f;
    if (IsKeyDown(KEY_LEFT)) {
        dx = -PlayerSpeedX;
    } else if (IsKeyDown(KEY_RIGHT)) {
        dx = PlayerSpeedX;
    }
    player->position.x += dx * delta_time;

    if (IsKeyDown(KEY_SPACE) && player->can_jump) {
        player->can_jump = false;
        player->dy = PlayerSpeedY;
    }

    if (player->can_jump) {
        player->position.y += player->dy * delta_time;
        player->dy += Gravity * delta_time;
    }
}

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void) {
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800 * 2;
    const int screenHeight = 450 * 2;

    Player player = {{PlayerOriginX, PlayerOriginY}, 0, true};

    // SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(screenWidth, screenHeight, "raylib [core] example - mouse input");

    SetTargetFPS(60);  // Set our game to run at 60 frames-per-second
    //---------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())  // Detect window close button or ESC key
    {
        //----------------------------------------------------------------------------------
        int screen_width = GetScreenWidth();
        int screen_height = GetScreenHeight();
        float delta_time = GetFrameTime();

        UpdatePlayer(&player, delta_time);

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

        ClearBackground(RAYWHITE);

        DrawBackground(screen_width, screen_height);
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