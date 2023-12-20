#include <stdlib.h>

#include "raylib.h"
#include "raymath.h"

static const int ppu = 16;

static const int OriginalWidth = 256;
static const int OriginalHeight = 240;
static const int ScalingFactor = 2;
#define SCALE(x) ((x)*ScalingFactor)
#define ScalingFactor (2)
#define RenderWidth (OriginalWidth * ScalingFactor)
#define RenderHeight (OriginalHeight * ScalingFactor)

#define OriginalWidthTiles (OriginalWidth / ppu)
#define OriginalHeightTiles (OriginalHeight / ppu)

static Texture DesertAtlas;
static Texture LinkWalkingAtlas;

typedef struct Entity {
    Vector2 position;
    float rotation;

    Vector2 speed;
    int speed_key;

    size_t sprite_idx;
    size_t sprite_idx_offset;
    bool flip_sprite;
} Entity;

static Entity Player;

static const uint8_t start_screen[11][16] = {
    {54, 54, 54, 54, 54, 54, 54, 23, 23, 54, 54, 54, 54, 54, 54, 54},
    {54, 54, 54, 54, 12, 54, 55, 23, 23, 54, 54, 54, 54, 54, 54, 54},
    {54, 54, 54, 55, 23, 23, 23, 23, 23, 54, 54, 54, 54, 54, 54, 54},
    {54, 54, 55, 23, 23, 23, 23, 23, 23, 54, 54, 54, 54, 54, 54, 54},
    {54, 55, 23, 23, 23, 23, 23, 23, 23, 53, 54, 54, 54, 54, 54, 54},
    {23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23},
    {52, 52, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 52, 52},
    {54, 54, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 54, 54},
    {54, 54, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 54, 54},
    {54, 54, 52, 52, 52, 52, 52, 52, 52, 52, 52, 52, 52, 52, 54, 54},
    {54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54},
};

///////////////////////////////////////////////////////////////////////////////////////
// Init.

static void PlayerInit(Entity* entity) {
    entity->position.x = OriginalWidth / 2;
    entity->position.y = OriginalHeight / 2;
    entity->sprite_idx = 0;
}

///////////////////////////////////////////////////////////////////////////////////////
// Update.

static void PlayerUpdate(Entity* entity, float delta_time) {
    static float distance = 8.5;

    size_t sprite_idx_offset = entity->sprite_idx_offset;
    bool flip_sprite = entity->flip_sprite;

    Vector2 new_speed = {0, 0};
    if (IsKeyDown(KEY_UP)) {
        new_speed = (Vector2){0, -1};
        sprite_idx_offset = 2;
        flip_sprite = false;
    }
    if (IsKeyDown(KEY_DOWN)) {
        new_speed = (Vector2){0, 1};
        sprite_idx_offset = 0;
        flip_sprite = false;
    }
    if (IsKeyDown(KEY_LEFT)) {
        new_speed = (Vector2){-1, 0};
        sprite_idx_offset = 1;
        flip_sprite = true;
    }
    if (IsKeyDown(KEY_RIGHT)) {
        new_speed = (Vector2){1, 0};
        sprite_idx_offset = 1;
        flip_sprite = false;
    }

    new_speed = Vector2Scale(new_speed, 70 * delta_time);
    distance -= Vector2LengthSqr(new_speed);
    if (distance <= 0.0) {
        distance = 8.5;
        entity->sprite_idx = (entity->sprite_idx + 1) % 2;
    }
    entity->position = Vector2Add(entity->position, new_speed);
    entity->sprite_idx_offset = sprite_idx_offset;
    entity->flip_sprite = flip_sprite;
}

///////////////////////////////////////////////////////////////////////////////////////
// Draw.

static void TileMapDrawDebug() {
    int idx = 0;
    for (size_t i = 0; i < OriginalHeightTiles; ++i, ++idx) {
        for (size_t j = 0; j < OriginalWidthTiles; ++j, ++idx) {
            Color color = idx % 2 == 0 ? BLACK : RED;
            DrawRectangleV((Vector2){SCALE(j * ppu), SCALE(i * ppu)},
                           (Vector2){SCALE((j * ppu) + ppu), SCALE((i * ppu) + ppu)}, color);
        }
    }
}

static void TileMapDraw() {
    int offset = ppu * 4;
    for (size_t i = 0; i < OriginalHeightTiles; ++i) {
        for (size_t j = 0; j < OriginalWidthTiles; ++j) {
            uint8_t tile_idx = start_screen[i][j];
            int x = (tile_idx % 8) * ppu;
            int y = (tile_idx / 8) * ppu;
            DrawTexturePro(
                DesertAtlas, (Rectangle){.x = x, .y = y, .width = ppu, .height = ppu},
                (Rectangle){
                    .x = SCALE(j * ppu), .y = SCALE((i * ppu) + offset), .width = SCALE(ppu), .height = SCALE(ppu)},
                (Vector2){0, 0}, 0, WHITE);
        }
    }
}

static void PlayerDraw(Entity* entity) {
    int flip = entity->flip_sprite ? -1 : 1;
    DrawTexturePro(
        LinkWalkingAtlas,
        (Rectangle){.x = (entity->sprite_idx_offset * ppu * 2) + (entity->sprite_idx * ppu),
                    .y = 0,
                    .width = flip * ppu,
                    .height = ppu},
        (Rectangle){
            .x = SCALE(entity->position.x), .y = SCALE(entity->position.y), .width = SCALE(ppu), .height = SCALE(ppu)},
        (Vector2){0, 0}, 0, WHITE);
    // DrawRectangle(SCALE(entity->position.x), SCALE(entity->position.y), SCALE(ppu), SCALE(ppu), GREEN);
}

///////////////////////////////////////////////////////////////////////////////////////

static void GameInit() {
    InitWindow(RenderWidth, RenderHeight, "Game");
    SetTargetFPS(60);

    DesertAtlas = LoadTexture("assets/textures/desert_atlas.png");
    assert(IsTextureReady(DesertAtlas));
    LinkWalkingAtlas = LoadTexture("assets/textures/link_walking_atlas.png");
    assert(IsTextureReady(LinkWalkingAtlas));
    // SetTextureFilter(DesertAtlas, TE)

    PlayerInit(&Player);
}

static void GameUpdate(float delta_time) { PlayerUpdate(&Player, delta_time); }

static void GameDraw() {
    BeginDrawing();

    ClearBackground(BLACK);

    if (false) {
        TileMapDrawDebug();
    }

    TileMapDraw();
    PlayerDraw(&Player);

    EndDrawing();
}

static void GameLoop() {
    float delta_time = GetFrameTime();
    GameUpdate(delta_time);
    GameDraw();
}

static void GameClose() { CloseWindow(); }

int main(void) {
    GameInit();
    while (!WindowShouldClose()) {
        GameLoop();
    }
    GameClose();

    return 0;
}