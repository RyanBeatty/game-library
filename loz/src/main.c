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
#define HudOffset (ppu * 4)

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

static const uint8_t StartScreenTileMap[11][16] = {
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

static const uint8_t StartScreenHitMap[11][16] = {
    {1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1},  // prevent lint.
    {1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1},  //
    {1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1},  //
    {1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1},  //
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1},  //
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},  //
    {1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1},  //
    {1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1},  //
    {1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1},  //
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},  //
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},  //
};

///////////////////////////////////////////////////////////////////////////////////////
// Init.

static void PlayerInit(Entity* entity) {
    entity->position.x = (OriginalWidth / 2) - ppu;
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
    Vector2 new_position = Vector2Add(entity->position, new_speed);

    for (size_t i = 0; i < OriginalHeightTiles; ++i) {
        for (size_t j = 0; j < OriginalWidthTiles; ++j) {
            if (StartScreenHitMap[i][j]) {
                Vector3 player_min = {new_position.x, new_position.y, 0};
                BoundingBox player_box = {.min = player_min, .max = Vector3Add(player_min, (Vector3){ppu, ppu, 0})};
                // Prevent top of player (link head) from colliding with scene so player can exit left/right.
                player_box.min.y += ppu / 2;
                Vector3 tile_min = {j * ppu, i * ppu, 0};
                BoundingBox tile_box = {.min = tile_min, .max = Vector3Add(tile_min, (Vector3){ppu, ppu, 0})};
                if (CheckCollisionBoxes(player_box, tile_box)) {
                    new_position = entity->position;
                    goto ret;
                }
            }
        }
    }

    // Offsets are so that player sprite stops while still onscreen.
    new_position.x = Clamp(new_position.x, 0, OriginalWidth - ppu);
    new_position.y = Clamp(new_position.y, 0, OriginalHeight - ppu);

ret:
    entity->position = new_position;
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
            uint8_t tile_idx = StartScreenTileMap[i][j];
            int x = (tile_idx % 8) * ppu;
            int y = (tile_idx / 8) * ppu;
            DrawTexturePro(
                DesertAtlas, (Rectangle){.x = x, .y = y, .width = ppu, .height = ppu},
                (Rectangle){
                    .x = SCALE(j * ppu), .y = SCALE((i * ppu) + offset), .width = SCALE(ppu), .height = SCALE(ppu)},
                (Vector2){0, 0}, 0, WHITE);
        }
    }

    if (true) {
        for (size_t i = 0; i < OriginalHeightTiles; ++i) {
            for (size_t j = 0; j < OriginalWidthTiles; ++j) {
                if (StartScreenHitMap[i][j]) {
                    DrawRectangleLines(SCALE(j * ppu), SCALE((i * ppu) + offset), SCALE(ppu), SCALE(ppu), RED);
                }
            }
        }
    }
}

static void PlayerDraw(Entity* entity) {
    int flip = entity->flip_sprite ? -1 : 1;
    DrawTexturePro(LinkWalkingAtlas,
                   (Rectangle){.x = (entity->sprite_idx_offset * ppu * 2) + (entity->sprite_idx * ppu),
                               .y = 0,
                               .width = flip * ppu,
                               .height = ppu},
                   (Rectangle){.x = SCALE(entity->position.x),
                               .y = SCALE(entity->position.y + HudOffset),
                               .width = SCALE(ppu),
                               .height = SCALE(ppu)},
                   (Vector2){0, 0}, 0, WHITE);
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