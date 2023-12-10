#include <stdlib.h>

#include "raylib.h"
#include "raymath.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Engine.

#define SCALING_FACTOR (3)
#define SCREEN_WIDTH_INIT (144 * SCALING_FACTOR)
#define SCREEN_HEIGHT_INIT (254 * SCALING_FACTOR)

static const float HorizontalScrollSpeed = 80;
static const float Gravity = 200;
static const float JumpSpeed = 100;

struct GameState;

typedef struct Entity {
    Vector3 position;
    float rotation_angle;  // radians.

    void (*update_func)(struct Entity*, struct GameState*);
    void (*draw_func)(struct Entity*, struct GameState*);
} Entity;

typedef struct GameState {
    float delta_time;
    int screen_width;
    int screen_height;

    Entity** entities;  // stb_array.
    Texture atlas;
} GameState;

static void GameStateUpdate(GameState* game_state) {
    game_state->delta_time = GetFrameTime();
    game_state->screen_width = GetScreenWidth();
    game_state->screen_height = GetScreenHeight();

    Entity* entity;
    for (size_t i = 0; i < arrlen(game_state->entities); ++i) {
        entity = game_state->entities[i];
        if (entity->update_func == NULL) {
            continue;
        }
        entity->update_func(entity, game_state);
    }
}

static void GameStateDraw(GameState* game_state) {
    BeginDrawing();
    ClearBackground(RAYWHITE);

    Entity* entity;
    for (size_t i = 0; i < arrlen(game_state->entities); ++i) {
        entity = game_state->entities[i];
        if (entity->draw_func == NULL) {
            continue;
        }
        entity->draw_func(entity, game_state);
    }

    EndDrawing();
}

static void GameStateClose(GameState* game_state) {
    UnloadTexture(game_state->atlas);
    CloseWindow();
}

//////////////////////////////////////////////////////////////////////
// Background.

typedef struct Background {
    Entity entity;
    Color background_color;
} Background;

static void BackgroundDraw(Entity* entity, GameState* game_state) {
    Background* background = (Background*)entity;
    Vector3 position = background->entity.position;
    DrawTexturePro(
        game_state->atlas, (Rectangle){.x = 0, .y = 0, .width = 144, .height = 254},
        (Rectangle){
            .x = position.x, .y = position.y, .width = game_state->screen_width, .height = game_state->screen_height},
        (Vector2){0, 0}, 0, WHITE);
}

static void BackgroundInit(Background* background) {
    background->entity.position = (Vector3){.x = 0, .y = 0, .z = 0};
    background->entity.rotation_angle = 0;
    background->entity.update_func = NULL;
    background->entity.draw_func = &BackgroundDraw;
    background->background_color = BROWN;
}

//////////////////////////////////////////////////////////////////////
// Player.
typedef struct Player {
    Entity entity;
    Vector3 speed;
} Player;

static void PlayerUpdate(Entity* entity, GameState* game_state) {
    Player* player = (Player*)entity;

    if (IsKeyPressed(KEY_SPACE)) {
        player->speed.y = -JumpSpeed;
    }

    player->entity.position.y += player->speed.y * game_state->delta_time;
    player->speed.y += Gravity * game_state->delta_time;
}

static void PlayerDraw(Entity* entity, GameState* game_state) {
    Player* player = (Player*)entity;
    Vector3 position = player->entity.position;

    DrawTexturePro(
        game_state->atlas, (Rectangle){.x = 314, .y = 0, .width = 17, .height = 12},
        (Rectangle){.x = position.x, .y = position.y, .width = 17 * SCALING_FACTOR, .height = 12 * SCALING_FACTOR},
        (Vector2){0, 0}, 0, WHITE);
    // Vector3 min = Vector3Add(entity->collision_rect.min, entity->position);
    // Vector3 max = Vector3Add(entity->collision_rect.max, min);
    // DrawRectangleLines(min.x, min.y, max.x - min.x, max.y - min.y, RED);
}

static void PlayerInit(Player* player, Vector3 position) {
    player->entity.position = position;
    player->entity.update_func = &PlayerUpdate;
    player->entity.draw_func = &PlayerDraw;
    player->speed = (Vector3){0, 0, 0};
}

//////////////////////////////////////////////////////////////////////
// Obstacle.

typedef struct Obstacle {
    Entity entity;
} Obstacle;

static void ObstacleInit(Obstacle* obstacle, Vector3 position);

static void ObstacleUpdate(Entity* entity, GameState* game_state) {
    Obstacle* obstacle = (Obstacle*)entity;
    if (entity->position.x <= -(26 * SCALING_FACTOR)) {
        ObstacleInit(obstacle, (Vector3){.x = game_state->screen_width, .y = 0, .z = 0});
    }
    entity->position.x -= HorizontalScrollSpeed * game_state->delta_time;
}

static void ObstacleDraw(Entity* entity, GameState* game_state) {
    Obstacle* obstacle = (Obstacle*)entity;
    Vector3 position = entity->position;

    DrawTexturePro(
        game_state->atlas, (Rectangle){.x = 288, .y = 0, .width = 26, .height = 160},
        (Rectangle){.x = position.x, .y = position.y, .width = 26 * SCALING_FACTOR, .height = 160 * SCALING_FACTOR},
        (Vector2){0, 0}, 0, WHITE);

    DrawTexturePro(game_state->atlas, (Rectangle){.x = 288, .y = 0, .width = 26, .height = 160},
                   (Rectangle){.x = position.x,
                               .y = position.y + (160 * SCALING_FACTOR) + 120 + (160 * SCALING_FACTOR),
                               .width = 26 * SCALING_FACTOR,
                               .height = 160 * SCALING_FACTOR},
                   (Vector2){26 * SCALING_FACTOR, 0}, -180, WHITE);
}

static void ObstacleInit(Obstacle* obstacle, Vector3 position) {
    obstacle->entity.position = position;
    obstacle->entity.draw_func = &ObstacleDraw;
    obstacle->entity.update_func = &ObstacleUpdate;
    obstacle->entity.position.y -= GetRandomValue(0, 100 * SCALING_FACTOR);
}

//////////////////////////////////////////////////////////////////////
// ObstacleSpawner.

// typedef struct Spawner {
//     Entity entity;
//     Obstacle* pipe_up;
//     Obstacle* pipe_down;
// } Spawner;

// static void SpawnerUpdate(Entity* entity, GameState* game_state) {
//     Spawner* spawner = (Spawner*)entity;
//     if (spawner->pipe_up->entity.position.x <= -(26 * SCALING_FACTOR)) {
//         SpawnerInit(spawner, spawner->pipe_up, spawner->pipe_down);
//     }
// }

// static void SpawnerInit(Spawner* spawner, Obstacle* pipe_up, Obstacle* pipe_down) {
//     spawner->pipe_up = pipe_up;
//     spawner->pipe_down = pipe_down;
//     spawner->entity.update_func = &SpawnerUpdate;

//     spawner->pipe_up->entity.position.x =
// }

//////////////////////////////////////////////////////////////////////

static void GameStateInit(GameState* game_state) {
    // SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(SCREEN_WIDTH_INIT, SCREEN_HEIGHT_INIT, "Game");

    SetTargetFPS(60);

    game_state->screen_width = GetScreenWidth();
    game_state->screen_height = GetScreenHeight();
    game_state->entities = NULL;

    Background* background = (Background*)calloc(1, sizeof(Background));
    BackgroundInit(background);
    arrpush(game_state->entities, (Entity*)background);

    Obstacle* obstacle = (Obstacle*)calloc(1, sizeof(Obstacle));
    ObstacleInit(obstacle, (Vector3){.x = game_state->screen_width, .y = 0, .z = 0});
    arrpush(game_state->entities, (Entity*)obstacle);

    Player* player = (Player*)calloc(1, sizeof(Player));
    PlayerInit(player,
               (Vector3){(game_state->screen_width / 2) - (17 * SCALING_FACTOR), (game_state->screen_height / 2), 0});
    arrpush(game_state->entities, (Entity*)player);

    game_state->atlas = LoadTexture("assets/atlas.png");
    SetTextureFilter(game_state->atlas, TEXTURE_FILTER_TRILINEAR);
}

int main(void) {
    GameState game_state;
    GameStateInit(&game_state);

    while (!WindowShouldClose()) {
        GameStateUpdate(&game_state);
        GameStateDraw(&game_state);
    }

    GameStateClose(&game_state);
    return 0;
}