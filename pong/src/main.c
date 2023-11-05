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

#include <stdint.h>

#include "raylib.h"
#include "raymath.h"

static const int ScreenWidth = 800;
static const int ScreenHeight = 450;

#define DistanceBetweenPlayers (ScreenWidth * 0.70f)
#define HalfDistanceBetweenPlayers (DistanceBetweenPlayers / 2.0f)

#define WALL_THICKNESS (10.0f)

static const int PaddleWidth = 20;
static const int PaddleHeight = 50;
#define HalfPaddleHeight (PaddleHeight / 2.0f)
#define PlayerDy (ScreenHeight * 0.20f)

typedef enum WallHittableSides {
    WHS_NONE = 0,
    WHS_TOP = 1,
    WHS_RIGHT = 1 << 1,
    WHS_BOTTOM = 1 << 2,
    WHS_LEFT = 1 << 3,
    WHS_ALL = (WHS_TOP | WHS_RIGHT | WHS_BOTTOM | WHS_LEFT),
} WallHittableSides;

typedef struct Wall {
    Rectangle rect;
    uint32_t hittable_sides_flag;
} Wall;

static Vector2 WallTopLeft(Wall wall) { return (Vector2){wall.rect.x, wall.rect.y}; }
static Vector2 WallTopRight(Wall wall) { return (Vector2){wall.rect.x + wall.rect.width, wall.rect.y}; }
static Vector2 WallBottomRight(Wall wall) {
    return (Vector2){wall.rect.x + wall.rect.width, wall.rect.y + wall.rect.height};
}
static Vector2 WallBottomLeft(Wall wall) { return (Vector2){wall.rect.x, wall.rect.y + wall.rect.height}; }

typedef struct Player {
    Vector2 paddle_pos;

} Player;

static int BallRadius = 10;

typedef struct Ball {
    Vector2 pos;
    Vector2 velocity;
} Ball;

static void DrawPlayerPaddle(Vector2 paddle_pos) {
    // TODO: Double check the displacement math here.
    DrawRectangle(paddle_pos.x, paddle_pos.y - HalfPaddleHeight, PaddleWidth, PaddleHeight, BLACK);
}

static void DrawBall(Vector2 pos) { DrawCircleV(pos, BallRadius, BLACK); }

static void DrawWalls(Wall* walls) {
    for (size_t i = 0; i < arrlen(walls); ++i) {
        DrawRectangleRec(walls[i].rect, GRAY);
    }
}

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

static void UpdateBall(Ball* ball, const Player player1, const Player player2, const float delta_time, Wall* walls) {
    // Debug. just to be able to speed up or slow down ball on the fly.
    if (IsKeyDown(KEY_ONE)) {
        ball->velocity = Vector2Scale(ball->velocity, 1.1);
    }
    if (IsKeyDown(KEY_TWO)) {
        ball->velocity = Vector2Scale(ball->velocity, 0.9);
    }

    Vector2 new_velocity = ball->velocity;

    Vector2 new_pos = ball->pos;
    new_pos.x += ball->velocity.x * delta_time;
    new_pos.y += ball->velocity.y * delta_time;

    for (size_t i = 0; i < arrlen(walls); ++i) {
        Wall wall = walls[i];
        if (CheckCollisionCircleRec(new_pos, BallRadius, wall.rect)) {
            float thickness = BallRadius + WALL_THICKNESS;
            new_pos.x = Clamp(new_pos.x, thickness, ScreenWidth - thickness);
            new_pos.y = Clamp(new_pos.y, thickness, ScreenHeight - thickness);

            Vector2 wall_top_left = WallTopLeft(wall);
            Vector2 wall_bottom_right = WallBottomRight(wall);
            bool collision_top = (wall.hittable_sides_flag && WHS_TOP) && (new_pos.y + BallRadius <= wall_top_left.y);
            bool collision_right =
                (wall.hittable_sides_flag && WHS_RIGHT) && (new_pos.x + BallRadius >= wall_bottom_right.x);
            bool collision_bottom =
                (wall.hittable_sides_flag && WHS_BOTTOM) && (new_pos.y + BallRadius >= wall_bottom_right.y);
            bool collision_left = (wall.hittable_sides_flag && WHS_LEFT) && (new_pos.x + BallRadius <= wall_top_left.x);

            Vector2 points[2];
            if (collision_top) {
                points[0] = wall_top_left;
                points[1] = WallTopRight(wall);
            } else if (collision_right) {
                points[0] = WallTopRight(wall);
                points[1] = wall_bottom_right;
            } else if (collision_bottom) {
                points[0] = wall_bottom_right;
                points[1] = WallBottomLeft(wall);
            } else if (collision_left) {
                points[0] = WallBottomLeft(wall);
                points[1] = wall_top_left;
            }
            Vector2 normal = Vector2Subtract(points[1], points[0]);
            normal = (Vector2){-normal.y, normal.x};
            normal = Vector2Normalize(normal);
            new_velocity = Vector2Reflect(new_velocity, normal);
            break;
        }
    }

    ball->pos = new_pos;
    ball->velocity = new_velocity;
}

static void Update(Player* player1, Player* player2, Ball* ball, const float delta_time, Wall* walls) {
    UpdatePlayer(player1, KEY_W, KEY_S, delta_time);
    UpdatePlayer(player2, KEY_UP, KEY_DOWN, delta_time);
    UpdateBall(ball, *player1, *player2, delta_time, walls);
}

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void) {
    // Initialization
    //--------------------------------------------------------------------------------------
    Player player1 = {{(ScreenWidth / 2.0f) - HalfDistanceBetweenPlayers - PaddleWidth, ScreenHeight / 2.0f}};
    Player player2 = {{(ScreenWidth / 2.0f) + HalfDistanceBetweenPlayers, ScreenHeight / 2.0f}};

    Ball ball = {{ScreenWidth / 2.0f, ScreenHeight / 2.0f}, {GetRandomValue(2, 10), GetRandomValue(2, 10)}};

    InitWindow(ScreenWidth, ScreenHeight, "raylib [core] example - mouse input");

    Wall* walls = NULL;
    arrpush(walls, ((Wall){{0.0f, 0.0f, ScreenWidth, WALL_THICKNESS}, WHS_ALL}));
    arrpush(walls, ((Wall){{0.0f, 0.0f, WALL_THICKNESS, ScreenHeight}, WHS_ALL}));
    arrpush(walls, ((Wall){{0.0f, ScreenHeight - WALL_THICKNESS, ScreenWidth, WALL_THICKNESS}, WHS_ALL}));
    arrpush(walls, ((Wall){{ScreenWidth - WALL_THICKNESS, 0.0f, WALL_THICKNESS, ScreenHeight}, WHS_ALL}));

    SetTargetFPS(60);  // Set our game to run at 60 frames-per-second
    //---------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())  // Detect window close button or ESC key
    {
        //----------------------------------------------------------------------------------
        float delta_time = GetFrameTime();

        Update(&player1, &player2, &ball, delta_time, walls);

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

        ClearBackground(RAYWHITE);

        // DrawText("move ball with mouse and click mouse button to change color", 10, 10, 20, DARKGRAY);
        DrawPlayerPaddle(player1.paddle_pos);
        DrawPlayerPaddle(player2.paddle_pos);
        DrawBall(ball.pos);
        DrawWalls(walls);

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow();  // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}