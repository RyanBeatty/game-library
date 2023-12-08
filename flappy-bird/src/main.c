#include <stdlib.h>

#include "raylib.h"
#include "raymath.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Engine.

#define SCREEN_WIDTH_INIT (144 * 3)
#define SCREEN_HEIGHT_INIT (254 * 3)

static const float GRAVITY = 98;
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

static void NoUpdate(Entity* entity, GameState* game_state) {}
static void NoDraw(Entity* entity, GameState* game_state) {}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct Background {
    Entity entity;
    Color background_color;
} Background;

static void BackgroundDraw(Entity* entity, GameState* game_state) {
    Background* background = (Background*)entity;
    Vector3 position = background->entity.position;
    // DrawRectangle(position.x, position.y, game_state->screen_width, game_state->screen_height,
    //               background->background_color);
    DrawTexturePro(
        game_state->atlas, (Rectangle){.x = 0, .y = 0, .width = 144, .height = 254},
        (Rectangle){
            .x = position.x, .y = position.y, .width = game_state->screen_width, .height = game_state->screen_height},
        (Vector2){0, 0}, 0, WHITE);
}

static void BackgroundInit(Background* background) {
    background->entity.position = (Vector3){.x = 0, .y = 0, .z = 0};
    background->entity.rotation_angle = 0;
    background->entity.update_func = &NoUpdate;
    background->entity.draw_func = &BackgroundDraw;
    background->background_color = BROWN;
}

typedef struct Player {
    Entity enity;
    Vector3 speed;
} Player;

static void PlayerUpdate(Entity* entity, GameState* game_state) {
    Player* player = (Player*)entity;

    if (IsKeyPressed(KEY_SPACE)) {
        player->speed.y = -JumpSpeed;
    }

    player->enity.position.y += player->speed.y * game_state->delta_time;
    player->speed.y += GRAVITY * game_state->delta_time;
}

static void PlayerDraw(Entity* entity, GameState* game_state) {
    Player* player = (Player*)entity;
    Vector3 position = player->enity.position;

    DrawTexturePro(game_state->atlas, (Rectangle){.x = 288, .y = 0, .width = 17, .height = 12},
                   (Rectangle){.x = position.x, .y = position.y, .width = 17 * 3, .height = 12 * 3}, (Vector2){0, 0}, 0,
                   WHITE);
}

static void PlayerInit(Player* player) {
    player->enity.position = (Vector3){.x = 50, .y = 50, .z = 0};
    player->enity.update_func = &PlayerUpdate;
    player->enity.draw_func = &PlayerDraw;
    player->speed = (Vector3){0, 0, 0};
}

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

    Player* player = (Player*)calloc(1, sizeof(Player));
    PlayerInit(player);
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