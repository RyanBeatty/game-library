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
#include <stdio.h>

#include "raylib.h"
#include "raymath.h"

static const int ScreenWidth = 600;
static const int ScreenHeight = 400;

#define DistanceBetweenPlayers (ScreenWidth * 0.70f)
#define HalfDistanceBetweenPlayers (DistanceBetweenPlayers / 2.0f)

#define WALL_THICKNESS (10.0f)

static const int PaddleWidth = 10;
static const int PaddleHeight = 50;
#define HalfPaddleHeight (PaddleHeight / 2.0f)
#define PlayerDy (ScreenHeight * 0.20f)

#define Vec2To3(vec) ((Vector3){vec.x, vec.y, 0.0f})
#define Vec3To2(vec) ((Vector2){vec.x, vec.y})

typedef struct Score {
    // First point entry means nothing.
    uint32_t points[3];
} Score;

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
    Vector2 velocity;
    uint32_t hittable_sides_flag;
} Wall;

static Vector2 WallTopLeft(Wall wall) { return (Vector2){wall.rect.x, wall.rect.y}; }
static Vector2 WallTopRight(Wall wall) { return (Vector2){wall.rect.x + wall.rect.width, wall.rect.y}; }
static Vector2 WallBottomRight(Wall wall) {
    return (Vector2){wall.rect.x + wall.rect.width, wall.rect.y + wall.rect.height};
}
static Vector2 WallBottomLeft(Wall wall) { return (Vector2){wall.rect.x, wall.rect.y + wall.rect.height}; }

typedef struct Player {
    Wall paddle;
} Player;

static int BallRadius = 5;

typedef struct Ball {
    Vector2 pos;
    Vector2 velocity;
} Ball;

static void DrawBall(Vector2 pos) { DrawCircleV(pos, BallRadius, BLACK); }

static void DrawWalls(Wall* walls) {
    for (size_t i = 0; i < arrlen(walls); ++i) {
        DrawRectangleRec(walls[i].rect, GRAY);
    }
}

static void DrawScore(Score score) {
    char p1_txt[100];
    sprintf(p1_txt, "%d", score.points[1]);
    DrawText(p1_txt, (ScreenWidth / 2.0) - 100, ScreenHeight * 0.15, 50, GRAY);

    char p2_txt[100];
    sprintf(p2_txt, "%d", score.points[2]);
    DrawText(p2_txt, (ScreenWidth / 2.0) + 50, ScreenHeight * 0.15, 50, GRAY);
}

static void Draw(Ball ball, Wall* walls, Score score) {
    BeginDrawing();
    ClearBackground(RAYWHITE);

    DrawBall(ball.pos);
    DrawWalls(walls);
    DrawScore(score);

    EndDrawing();
}

static void UpdatePlayer(Player* player, int key_up, int key_down, float delta_time) {
    Vector2 velocity = {0.0f, 0.0f};
    if (IsKeyDown(key_up)) {
        velocity.y += -PlayerDy * delta_time;
    }
    if (IsKeyDown(key_down)) {
        velocity.y += PlayerDy * delta_time;
    }

    player->paddle.velocity = velocity;
    player->paddle.rect.y += velocity.y;
    // Bounds checking for playable space.
    player->paddle.rect.y = player->paddle.rect.y < WALL_THICKNESS ? WALL_THICKNESS : player->paddle.rect.y;
    player->paddle.rect.y = player->paddle.rect.y + PaddleHeight > ScreenHeight - WALL_THICKNESS
                                ? ScreenHeight - PaddleHeight - WALL_THICKNESS
                                : player->paddle.rect.y;
}

static uint32_t UpdateBall(Ball* ball, const Player player1, const Player player2, const float delta_time,
                           Wall* walls) {
    uint32_t scored = 0;
    // Debug. just to be able to speed up or slow down ball on the fly.
    if (IsKeyDown(KEY_ONE)) {
        ball->velocity = Vector2Scale(ball->velocity, 1.1);
    }
    if (IsKeyDown(KEY_TWO)) {
        ball->velocity = Vector2Scale(ball->velocity, 0.9);
    }

    Vector2 ball_radius = {BallRadius, 0.0f};
    Vector2 new_velocity = ball->velocity;
    float angle = Vector2Angle(ball_radius, new_velocity);
    // Extend ball movement when calculating collisions by the radius of the ball in the direction we moved (angle of
    // velocity).
    ball_radius = Vector2Rotate(ball_radius, -(RAD2DEG * angle));
    Vector2 old_pos = ball->pos;
    Vector2 new_pos = Vector2Add(old_pos, Vector2Scale(new_velocity, delta_time));
    Vector2 new_pos_with_radius = Vector2Add(new_pos, ball_radius);
    breakpoint();

    for (size_t i = 0; i < arrlen(walls); ++i) {
        Wall wall = walls[i];

        // For each wall, check if we've hit any of the sides.
        for (size_t j = 0; j < 4; ++j) {
            uint32_t side_mask = WHS_NONE;
            Vector2 side_start;
            Vector2 side_end;
            if (j == 0) {
                side_mask = WHS_TOP;
                side_end = WallTopRight(wall);
                side_start = WallTopLeft(wall);
            } else if (j == 1) {
                side_mask = WHS_RIGHT;
                side_start = WallTopRight(wall);
                side_end = WallBottomRight(wall);
            } else if (j == 2) {
                side_mask = WHS_BOTTOM;
                side_start = WallBottomRight(wall);
                side_end = WallBottomLeft(wall);
            } else if (j == 3) {
                side_mask = WHS_LEFT;
                side_start = WallTopLeft(wall);
                side_end = WallBottomLeft(wall);
            }

            if (!(wall.hittable_sides_flag && side_mask)) {
                continue;
            }

            Vector2 collision;
            if (CheckCollisionLines(old_pos, new_pos_with_radius, side_start, side_end, &collision)) {
                new_pos = Vector2Add(collision, Vector2Negate(ball_radius));
                Vector2 normal = Vector2Subtract(side_end, side_start);
                normal = (Vector2){-normal.y, normal.x};
                normal = Vector2Normalize(normal);
                new_velocity = Vector2Reflect(new_velocity, normal);
                goto ret;
            }
        }
    }

    if (new_pos.x + BallRadius <= 0.0f) {
        scored = 1;
    } else if (new_pos.x - BallRadius >= ScreenWidth) {
        scored = 2;
    }

ret:
    ball->pos = new_pos;
    ball->velocity = new_velocity;
    return scored;
}

static void UpdateScore(Score* score, uint32_t scored) { ++score->points[scored]; }

static uint32_t Update(Player* player1, Player* player2, Ball* ball, const float delta_time, Wall* walls,
                       Score* score) {
    UpdatePlayer(player1, KEY_W, KEY_S, delta_time);
    UpdatePlayer(player2, KEY_UP, KEY_DOWN, delta_time);
    walls[0] = player1->paddle;
    walls[1] = player2->paddle;
    uint32_t scored = UpdateBall(ball, *player1, *player2, delta_time, walls);
    UpdateScore(score, scored);
    return scored;
}

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void) {
    // Initialization
    //--------------------------------------------------------------------------------------

    Score score = {{0, 0, 0}};

    // Ball ball = {{ScreenWidth / 2.0f, ScreenHeight / 2.0f},
    //              {10.0f, 50.0f}};  //{GetRandomValue(2, 10), GetRandomValue(2, 10)}};
    Ball ball = {{ScreenWidth / 2.0f, WALL_THICKNESS + 20}, {0, -10}};

    InitWindow(ScreenWidth, ScreenHeight, "raylib [core] example - mouse input");

    Wall* walls = NULL;
    Wall p1_paddle = {
        {(ScreenWidth / 2.0f) - HalfDistanceBetweenPlayers - PaddleWidth, (ScreenHeight / 2.0f) - HalfPaddleHeight,
         PaddleWidth, PaddleHeight},
        {(ScreenWidth / 2.0f) - HalfDistanceBetweenPlayers - PaddleWidth, (ScreenHeight / 2.0f) - HalfPaddleHeight},
        WHS_RIGHT};
    arrpush(walls, p1_paddle);
    Wall p2_paddle = {{(ScreenWidth / 2.0f) + HalfDistanceBetweenPlayers, (ScreenHeight / 2.0f) - HalfPaddleHeight,
                       PaddleWidth, PaddleHeight},
                      {(ScreenWidth / 2.0f) + HalfDistanceBetweenPlayers, (ScreenHeight / 2.0f) - HalfPaddleHeight},
                      WHS_LEFT};
    arrpush(walls, p2_paddle);
    arrpush(walls, ((Wall){{0.0f, 0.0f, ScreenWidth, WALL_THICKNESS}, {0.0f, 0.0f}, WHS_ALL}));
    arrpush(walls, ((Wall){{0.0f, ScreenHeight - WALL_THICKNESS, ScreenWidth, WALL_THICKNESS},
                           {0.0f, ScreenHeight - WALL_THICKNESS},
                           WHS_ALL}));

    Player player1 = {p1_paddle};
    Player player2 = {p2_paddle};

    SetTargetFPS(60);  // Set our game to run at 60 frames-per-second
    //---------------------------------------------------------------------------------------

    uint32_t scored = 0;

    // Main game loop
    while (!WindowShouldClose())  // Detect window close button or ESC key
    {
        //----------------------------------------------------------------------------------
        float delta_time = GetFrameTime();
        scored = Update(&player1, &player2, &ball, delta_time, walls, &score);
        Draw(ball, walls, score);
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow();  // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}