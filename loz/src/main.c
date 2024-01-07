#include <stdint.h>
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
static Texture LinkAtlas;

typedef enum Direction {
    DIR_UP = 0,
    DIR_RIGHT,
    DIR_DOWN,
    DIR_LEFT,

    DIR_COUNT
} Direction;

typedef struct Entity {
    Vector2 position;
    float rotation;

    Vector2 speed;
    int speed_key;

    // Animation info. Create a bounding box/rect for the animation and cycle through each frame.
    // Assumes each frame has same sized box/rect.
    Vector2 animation_rect_min;
    int animation_rect_width;
    int animation_rect_height;
    int animation_frame_offset;
    int animation_num_frames;
    Vector2 animation_dest_offset;
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

    entity->animation_rect_min = (Vector2){0, 0};
    entity->animation_rect_width = ppu;
    entity->animation_rect_height = ppu;
    entity->animation_frame_offset = 0;
    entity->animation_num_frames = 1;
    entity->animation_dest_offset = (Vector2){0, 0};
}

///////////////////////////////////////////////////////////////////////////////////////
// Update.

#define PLAYER_ATTACK_DURATION_SEC_INIT (0.4f)
#define PLAYER_ATTACK_COOLDOWN_SEC_INIT (0.2f)

typedef enum PlayerState {
    PS_IDLE = 0,
    PS_WALK,
    PS_ATTACK,

    PS_COUNT
} PlayerState;

static void PlayerUpdate(Entity* entity, float delta_time) {
    static PlayerState player_state = PS_IDLE;
    static Direction player_direction = DIR_DOWN;
    static float walk_cycle_distance = 8.5;

    static float attack_cooldown_sec = 0.0f;
    static float attack_duration_sec = PLAYER_ATTACK_DURATION_SEC_INIT;

    attack_cooldown_sec -= delta_time;
    attack_cooldown_sec = attack_cooldown_sec <= 0.0f ? 0.0f : attack_cooldown_sec;

    Vector2 new_speed = {0, 0};
    // TODO: Maybe rethink this state stuff.
    switch (player_state) {
        case PS_IDLE: {
            switch (player_direction) {
                case DIR_UP: {
                    // Set animation info.
                    entity->animation_rect_min = (Vector2){ppu * 4, 0};
                    entity->animation_rect_width = ppu;
                    entity->animation_rect_height = ppu;
                    entity->animation_dest_offset = (Vector2){0, 0};
                    entity->animation_num_frames = 2;
                    break;
                }
                case DIR_RIGHT: {
                    // Set animation info.
                    entity->animation_rect_min = (Vector2){ppu * 2, 0};
                    entity->animation_rect_width = ppu;
                    entity->animation_rect_height = ppu;
                    entity->animation_dest_offset = (Vector2){0, 0};
                    entity->animation_num_frames = 2;
                    break;
                }
                case DIR_DOWN: {
                    // Set animation info.
                    entity->animation_rect_min = (Vector2){0, 0};
                    entity->animation_rect_width = ppu;
                    entity->animation_rect_height = ppu;
                    entity->animation_dest_offset = (Vector2){0, 0};
                    entity->animation_num_frames = 2;
                    break;
                }
                case DIR_LEFT: {
                    // Set animation info.
                    entity->animation_rect_min = (Vector2){ppu * 2, 0};
                    entity->animation_rect_width = -ppu;
                    entity->animation_rect_height = ppu;
                    entity->animation_dest_offset = (Vector2){0, 0};
                    entity->animation_num_frames = 2;
                    break;
                }
                default: {
                    assert(false);
                }
            }
            // Note: purposefully fallthrough.
        }
        case PS_WALK: {
            if (IsKeyDown(KEY_UP)) {
                new_speed = (Vector2){0, -1};
                player_direction = DIR_UP;
                // Set animation info.
                entity->animation_rect_min = (Vector2){ppu * 4, 0};
                entity->animation_rect_width = ppu;
                entity->animation_rect_height = ppu;
                entity->animation_dest_offset = (Vector2){0, 0};
                entity->animation_num_frames = 2;
            }
            if (IsKeyDown(KEY_DOWN)) {
                new_speed = (Vector2){0, 1};
                player_direction = DIR_DOWN;
                // Set animation info.
                entity->animation_rect_min = (Vector2){0, 0};
                entity->animation_rect_width = ppu;
                entity->animation_rect_height = ppu;
                entity->animation_dest_offset = (Vector2){0, 0};
                entity->animation_num_frames = 2;
            }
            if (IsKeyDown(KEY_LEFT)) {
                new_speed = (Vector2){-1, 0};
                player_direction = DIR_LEFT;
                // Set animation info.
                entity->animation_rect_min = (Vector2){ppu * 2, 0};
                entity->animation_rect_width = -ppu;
                entity->animation_rect_height = ppu;
                entity->animation_dest_offset = (Vector2){0, 0};
                entity->animation_num_frames = 2;
            }
            if (IsKeyDown(KEY_RIGHT)) {
                new_speed = (Vector2){1, 0};
                player_direction = DIR_RIGHT;
                // Set animation info.
                entity->animation_rect_min = (Vector2){ppu * 2, 0};
                entity->animation_rect_width = ppu;
                entity->animation_rect_height = ppu;
                entity->animation_dest_offset = (Vector2){0, 0};
                entity->animation_num_frames = 2;
            }
            if (IsKeyDown(KEY_SPACE) && attack_cooldown_sec == 0.0f) {
                // Can't move while attacking.
                new_speed.x = 0;
                new_speed.y = 0;
                player_state = PS_ATTACK;
                attack_duration_sec = PLAYER_ATTACK_DURATION_SEC_INIT;

                switch (player_direction) {
                    case DIR_UP: {
                        entity->animation_rect_min = (Vector2){0, ppu * 4};
                        entity->animation_rect_width = ppu;
                        entity->animation_rect_height = ppu * 2;
                        entity->animation_frame_offset = 0;
                        entity->animation_dest_offset = (Vector2){0, -ppu};
                        entity->animation_num_frames = 4;
                        break;
                    }
                    case DIR_RIGHT: {
                        entity->animation_rect_min = (Vector2){0, ppu * 3};
                        entity->animation_rect_width = ppu * 2;
                        entity->animation_rect_height = ppu;
                        entity->animation_frame_offset = 0;
                        entity->animation_dest_offset = (Vector2){0, 0};
                        entity->animation_num_frames = 4;
                        break;
                    }
                    case DIR_DOWN: {
                        entity->animation_rect_min = (Vector2){0, ppu * 1};
                        entity->animation_rect_width = ppu;
                        entity->animation_rect_height = ppu * 2;
                        entity->animation_frame_offset = 0;
                        entity->animation_dest_offset = (Vector2){0, 0};
                        entity->animation_num_frames = 4;
                        break;
                    }
                    case DIR_LEFT: {
                        entity->animation_rect_min = (Vector2){0, ppu * 3};
                        entity->animation_rect_width = -(ppu * 2);
                        entity->animation_rect_height = ppu;
                        entity->animation_frame_offset = 0;
                        entity->animation_dest_offset = (Vector2){-ppu, 0};
                        entity->animation_num_frames = 4;
                        break;
                    }
                    default: {
                        assert(false);
                    }
                }
            }

            // If we are actually moving, ensure we update the animation frame offset if we have walked far enough.
            new_speed = Vector2Scale(new_speed, 70 * delta_time);
            walk_cycle_distance -= Vector2LengthSqr(new_speed);
            if (walk_cycle_distance <= 0.0) {
                walk_cycle_distance = 8.5;
                ++entity->animation_frame_offset;
            }
            break;
        }
        case PS_ATTACK: {
            // TODO: ugly ugly. just get animations working for now.
            float attack_animation_frame_time_sec = PLAYER_ATTACK_DURATION_SEC_INIT / entity->animation_num_frames;
            float attack_duration_count_up_from_zero_sec = fabs(attack_duration_sec - PLAYER_ATTACK_DURATION_SEC_INIT);
            entity->animation_frame_offset = attack_duration_count_up_from_zero_sec / attack_animation_frame_time_sec;

            attack_duration_sec -= delta_time;
            if (attack_duration_sec <= 0.0f) {
                attack_duration_sec = 0.0f;
                attack_cooldown_sec = PLAYER_ATTACK_COOLDOWN_SEC_INIT;
                player_state = PS_IDLE;
            }
            break;
        }
        default: {
            assert(false);
        }
    }

    // Ensure that we are always within the current animation bounds. For example, when an attack animation is done,
    // this will ensure we are at the starting animation of a walk animation if we then walk.
    entity->animation_frame_offset %= entity->animation_num_frames;

    // Move player.
    {
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
    }
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

static void TileMapDraw(Texture atlas, const uint8_t (*tile_map)[OriginalWidthTiles],
                        const uint8_t (*hit_map)[OriginalWidthTiles]) {
    int offset = ppu * 4;
    for (size_t i = 0; i < OriginalHeightTiles; ++i) {
        for (size_t j = 0; j < OriginalWidthTiles; ++j) {
            uint8_t tile_idx = tile_map[i][j];
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
                if (hit_map[i][j]) {
                    DrawRectangleLines(SCALE(j * ppu), SCALE((i * ppu) + offset), SCALE(ppu), SCALE(ppu), RED);
                }
            }
        }
    }
}

static void PlayerDraw(Entity* entity) {
    // Shift by abs(width) because width can be negative in the case where we are flipping the sprite.
    int abs_width = abs(entity->animation_rect_width);
    int x = entity->animation_rect_min.x + (entity->animation_frame_offset * abs_width);
    DrawTexturePro(LinkAtlas,
                   (Rectangle){.x = x,
                               .y = entity->animation_rect_min.y,
                               .width = entity->animation_rect_width,
                               .height = entity->animation_rect_height},
                   (Rectangle){.x = SCALE(entity->position.x + entity->animation_dest_offset.x),
                               .y = SCALE(entity->position.y + entity->animation_dest_offset.y + HudOffset),
                               .width = SCALE(abs_width),
                               .height = SCALE(entity->animation_rect_height)},
                   (Vector2){0, 0}, 0, WHITE);
}

///////////////////////////////////////////////////////////////////////////////////////

static void GameInit() {
    InitWindow(RenderWidth, RenderHeight, "Game");
    SetTargetFPS(60);

    DesertAtlas = LoadTexture("assets/textures/desert_atlas.png");
    assert(IsTextureReady(DesertAtlas));
    LinkAtlas = LoadTexture("assets/textures/LinkAtlas.png");
    assert(IsTextureReady(LinkAtlas));
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

    TileMapDraw(DesertAtlas, StartScreenTileMap, StartScreenHitMap);
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