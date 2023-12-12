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
    BoundingBox (*get_collision_rect_func)(struct Entity*);
    void (*resolve_collision_func)(struct Entity*, struct GameState*);
} Entity;

typedef struct GameState {
    float delta_time;
    int screen_width;
    int screen_height;
    bool active;

    Entity** entities;  // stb_array.
    Texture atlas;
} GameState;

static void GameStateCheckCollisions(GameState* game_state) {
    for (size_t i = 0; i < arrlen(game_state->entities); ++i) {
        for (size_t j = 1; j < arrlen(game_state->entities); ++j) {
            Entity* entity1 = game_state->entities[i];
            Entity* entity2 = game_state->entities[j];
            if (entity1 == entity2 || entity1->get_collision_rect_func == NULL ||
                entity2->get_collision_rect_func == NULL) {
                continue;
            }

            BoundingBox bb1 = entity1->get_collision_rect_func(entity1);
            BoundingBox bb2 = entity2->get_collision_rect_func(entity2);
            if (CheckCollisionBoxes(bb1, bb2)) {
                if (entity1->resolve_collision_func != NULL) {
                    entity1->resolve_collision_func(entity1, game_state);
                }
                if (entity2->resolve_collision_func != NULL) {
                    entity2->resolve_collision_func(entity2, game_state);
                }
            }
        }
    }
}

static void GameStateUpdate(GameState* game_state) {
    game_state->delta_time = GetFrameTime();
    game_state->screen_width = GetScreenWidth();
    game_state->screen_height = GetScreenHeight();

    if (game_state->active) {
        Entity* entity;
        for (size_t i = 0; i < arrlen(game_state->entities); ++i) {
            entity = game_state->entities[i];
            if (entity->update_func == NULL) {
                continue;
            }
            entity->update_func(entity, game_state);
        }

        GameStateCheckCollisions(game_state);
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

    if (true) {
        for (size_t i = 0; i < arrlen(game_state->entities); ++i) {
            Entity* entity = game_state->entities[i];
            if (entity->get_collision_rect_func == NULL) {
                continue;
            }
            BoundingBox bb = entity->get_collision_rect_func(entity);
            DrawRectangleLines(bb.min.x, bb.min.y, bb.max.x - bb.min.x, bb.max.y - bb.min.y, RED);
        }
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

static BoundingBox PlayerGetCollisionRect(Entity* entity) {
    BoundingBox bb = {.min = entity->position,
                      .max = Vector3Add(entity->position, (Vector3){17 * SCALING_FACTOR, 12 * SCALING_FACTOR, 0})};
    return bb;
}

static void PlayerResolveCollision(Entity* entity, GameState* game_state) { game_state->active = false; }

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
        game_state->atlas, (Rectangle){.x = 482, .y = 0, .width = 17, .height = 12},
        (Rectangle){.x = position.x, .y = position.y, .width = 17 * SCALING_FACTOR, .height = 12 * SCALING_FACTOR},
        (Vector2){0, 0}, 0, WHITE);
}

static void PlayerInit(Player* player, Vector3 position) {
    player->entity.position = position;
    player->entity.update_func = &PlayerUpdate;
    player->entity.draw_func = &PlayerDraw;
    player->entity.get_collision_rect_func = &PlayerGetCollisionRect;
    player->entity.resolve_collision_func = &PlayerResolveCollision;
    player->speed = (Vector3){0, 0, 0};
}

//////////////////////////////////////////////////////////////////////
// Pipe.

typedef struct Pipe {
    Entity entity;
} Pipe;

static void PipeInit(Pipe* pipe, Vector3 position);

static void PipeUpdate(Entity* entity, GameState* game_state) {
    Pipe* pipe = (Pipe*)entity;
    if (entity->position.x <= -(26 * SCALING_FACTOR)) {
        PipeInit(pipe, (Vector3){.x = game_state->screen_width, .y = 0, .z = 0});
    }
    entity->position.x -= HorizontalScrollSpeed * game_state->delta_time;
}

static void PipeDraw(Entity* entity, GameState* game_state) {
    Vector3 position = entity->position;

    DrawTexturePro(
        game_state->atlas, (Rectangle){.x = 288, .y = 0, .width = 26, .height = 160},
        (Rectangle){.x = position.x, .y = position.y, .width = 26 * SCALING_FACTOR, .height = 160 * SCALING_FACTOR},
        (Vector2){0, 0}, entity->rotation_angle, WHITE);
}

static BoundingBox PipeGetCollisionRect(Entity* entity) {
    Vector2 position = (Vector2){entity->position.x, entity->position.y};

    Vector2 tl = Vector2Rotate((Vector2){0, 0}, DEG2RAD * entity->rotation_angle);
    Vector2 tr = Vector2Rotate((Vector2){26 * SCALING_FACTOR, 0}, DEG2RAD * entity->rotation_angle);
    Vector2 bl = Vector2Rotate((Vector2){0, 160 * SCALING_FACTOR}, DEG2RAD * entity->rotation_angle);
    Vector2 br = Vector2Rotate((Vector2){26 * SCALING_FACTOR, 160 * SCALING_FACTOR}, DEG2RAD * entity->rotation_angle);
    tr = Vector2Add(position, tr);
    tl = Vector2Add(position, tl);
    bl = Vector2Add(position, bl);
    br = Vector2Add(position, br);

    float minX = fmin(tl.x, fmin(tr.x, fmin(bl.x, br.x)));
    float maxX = fmax(tl.x, fmax(tr.x, fmax(bl.x, br.x)));
    float minY = fmin(tl.y, fmin(tr.y, fmin(bl.y, br.y)));
    float maxY = fmax(tl.y, fmax(tr.y, fmax(bl.y, br.y)));
    Vector3 min = (Vector3){minX, minY, 0};
    Vector3 max = (Vector3){maxX, maxY, 0};
    BoundingBox bb = (BoundingBox){.min = min, .max = max};
    return bb;
}

static void PipeInit(Pipe* pipe, Vector3 position) {
    pipe->entity.position = position;
    pipe->entity.draw_func = &PipeDraw;
    pipe->entity.update_func = &PipeUpdate;
    pipe->entity.get_collision_rect_func = &PipeGetCollisionRect;
}

//////////////////////////////////////////////////////////////////////
// ObstacleSpawner.

typedef struct Spawner {
    Entity entity;
    Pipe* pipe_up;
    Pipe* pipe_down;
} Spawner;

static void SpawnerInit(Spawner*, Pipe*, Pipe*, int);

static void SpawnerUpdate(Entity* entity, GameState* game_state) {
    Spawner* spawner = (Spawner*)entity;
    if (spawner->pipe_up->entity.position.x <= -(26 * SCALING_FACTOR)) {
        SpawnerInit(spawner, spawner->pipe_up, spawner->pipe_down, game_state->screen_width);
    }
}

static void SpawnerInit(Spawner* spawner, Pipe* pipe_up, Pipe* pipe_down, int start_x) {
    spawner->pipe_up = pipe_up;
    spawner->pipe_down = pipe_down;
    spawner->entity.update_func = &SpawnerUpdate;

    Vector3 position = (Vector3){start_x, -GetRandomValue(0, 100 * SCALING_FACTOR), 0};
    PipeInit(pipe_up, position);

    position.x += 26 * SCALING_FACTOR;
    position.y += (160 * SCALING_FACTOR) + 120 + (160 * SCALING_FACTOR);
    pipe_down->entity.rotation_angle = -180;
    PipeInit(pipe_down, position);
}

//////////////////////////////////////////////////////////////////////

static void GameStateInit(GameState* game_state) {
    // SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(SCREEN_WIDTH_INIT, SCREEN_HEIGHT_INIT, "Game");

    SetTargetFPS(60);

    game_state->screen_width = GetScreenWidth();
    game_state->screen_height = GetScreenHeight();
    game_state->entities = NULL;
    game_state->active = true;

    Background* background = (Background*)calloc(1, sizeof(Background));
    BackgroundInit(background);
    arrpush(game_state->entities, (Entity*)background);

    Pipe* pipe = (Pipe*)calloc(1, sizeof(Pipe));
    arrpush(game_state->entities, (Entity*)pipe);
    Pipe* pipe2 = (Pipe*)calloc(1, sizeof(Pipe));
    arrpush(game_state->entities, (Entity*)pipe2);

    Spawner* spawner = (Spawner*)calloc(1, sizeof(Spawner));
    SpawnerInit(spawner, pipe, pipe2, game_state->screen_width);
    arrpush(game_state->entities, (Entity*)spawner);

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