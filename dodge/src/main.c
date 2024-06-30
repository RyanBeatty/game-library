#ifndef UNITY_BUILD
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "raylib.h"
#include "raymath.h"

#endif

const float RenderWidth = 320;
const float RenderHeight = 180;

static const Vector3 DOWN = {1, -1, 1};

////////////////////////////////////////////////////////////

typedef enum TextureLabel {
    TEXTURE_NONE = 0,
    TEXTURE_TILEMAP_CHARACTERS,
    TEXTURE_TILEMAP_BACKGROUND,
    TEXTURE_COUNT
} TextureLabel;

#define TEXTURE_CAP (TEXTURE_COUNT + 32)

Texture Textures[TEXTURE_CAP] = {0};

static void TextureCacheInit() {
    memset(&Textures, 0, ArraySizeOf(Textures) * sizeof(Textures[0]));

    {
        Texture texture = LoadTexture("assets/art/tilemap-characters.png");
        Assert(IsTextureReady(texture));
        Textures[TEXTURE_TILEMAP_CHARACTERS] = texture;
    }

    {
        Texture texture = LoadTexture("assets/art/8bit.png");
        Assert(IsTextureReady(texture));
        Textures[TEXTURE_TILEMAP_BACKGROUND] = texture;
    }
}

#define GetTexture(idx) (Textures[(idx)])

static Texture* AllocateTexture() {
    Texture* texture = NULL;
    for (uint32_t i = 0; i < ArraySizeOf(Textures); ++i) {
        Texture* t = &GetTexture(i);
        if (t->id == 0) {
            texture = t;
            break;
        }
    }
    Assert(texture != NULL);
    memset(texture, 0, sizeof(*texture));
    return texture;
}

static Rectangle TextureGetTile(Texture texture, int32_t tile_w, int32_t tile_h, int32_t spacing_x, int32_t spacing_y,
                                uint32_t idx) {
    uint32_t ncols = (texture.width + spacing_x) / (tile_w + spacing_x);
    uint32_t row = idx / ncols;
    uint32_t col = idx % ncols;
    return (Rectangle){
        .x = (tile_w + spacing_x) * col, .y = (tile_h + spacing_y) * row, .width = tile_w, .height = tile_h};
}

////////////////////////////////////////////////////////////

typedef enum SoundId {
    SOUND_NONE = 0,
    SOUND_BOMB_EXPLOSION,
    SOUND_PICKUP_COIN,
    SOUND_FOOTSTEP_LEFT,
    SOUND_FOOTSTEP_RIGHT,
    SOUND_GAME_OVER,
    SOUND_GAME_START,
    SOUND_COUNT
} SoundId;

Sound Sounds[SOUND_COUNT];

static void AudioSystemInit() {
    memset(&Sounds, 0, ArraySizeOf(Sounds) * sizeof(Sound));

    InitAudioDevice();

    Sounds[SOUND_BOMB_EXPLOSION] = LoadSound("assets/sound/bomb_explosion.wav");
    Assert(IsSoundReady(Sounds[SOUND_BOMB_EXPLOSION]));

    Sounds[SOUND_PICKUP_COIN] = LoadSound("assets/sound/pickup_coin.wav");
    Assert(IsSoundReady(Sounds[SOUND_PICKUP_COIN]));

    {
        Wave wave = LoadWave("assets/sound/footstep_left.wav");
        wave.sampleRate *= 2.7;
        Sounds[SOUND_FOOTSTEP_LEFT] = LoadSoundFromWave(wave);
        Assert(IsSoundReady(Sounds[SOUND_FOOTSTEP_LEFT]));
    }

    {
        Wave wave = LoadWave("assets/sound/footstep_right.wav");
        wave.sampleRate *= 2.7;
        Sounds[SOUND_FOOTSTEP_RIGHT] = LoadSoundFromWave(wave);
        Assert(IsSoundReady(Sounds[SOUND_FOOTSTEP_RIGHT]));
    }

    Sounds[SOUND_GAME_OVER] = LoadSound("assets/sound/game_over.wav");
    Assert(IsSoundReady(Sounds[SOUND_GAME_OVER]));

    Sounds[SOUND_GAME_START] = LoadSound("assets/sound/game_start.wav");
    Assert(IsSoundReady(Sounds[SOUND_GAME_START]));
}

#define GetSound(id) (Sounds[(id)])

static void PlaySoundIfNotPlaying(SoundId id) {
    if (!IsSoundPlaying(GetSound(id))) {
        PlaySound(GetSound(id));
    }
}

////////////////////////////////////////////////////////////

typedef enum ShapeKind { SHAPE_NONE = 0, SHAPE_REC, SHAPE_CIRCLE, SHAPE_COUNT } ShapeKind;

typedef struct Shape {
    ShapeKind kind;

    union {
        struct {
            float width;
            float height;
        };

        float radius;
    };
} Shape;

#define Shape2Rec(c, s) ((Rectangle){(c).x - ((s).width * 0.5f), (c).y - ((s).height * 0.5f), (s).width, (s).height})

static bool Collides(Vector2 c1, Shape s1, Vector2 c2, Shape s2) {
    if (s1.kind == SHAPE_NONE || s2.kind == SHAPE_NONE) {
        return false;
    }

    if (s2.kind < s1.kind) {
        Shape temp = s1;
        s1 = s2;
        s2 = temp;
        Vector2 ctemp = c1;
        c1 = c2;
        c2 = ctemp;
    }

    switch (s1.kind) {
        case SHAPE_REC: {
            switch (s2.kind) {
                case SHAPE_REC: {
                    return CheckCollisionRecs(Shape2Rec(c1, s1), Shape2Rec(c2, s2));
                }
                case SHAPE_CIRCLE: {
                    return CheckCollisionCircleRec(c2, s2.radius, Shape2Rec(c1, s1));
                }
                default: {
                    Assert(false);
                }
            }
        }
        case SHAPE_CIRCLE: {
            switch (s2.kind) {
                case SHAPE_REC: {
                    return CheckCollisionCircleRec(c1, s1.radius, Shape2Rec(c2, s2));
                }
                case SHAPE_CIRCLE: {
                    return CheckCollisionCircles(c1, s1.radius, c2, s2.radius);
                }
                default: {
                    Assert(false);
                }
            }
        }
        default: {
            Assert(false);
        }
    }
    return false;
}

////////////////////////////////////////////////////////////

typedef enum SpawnFlag { SFLAG_NONE = 0, SFLAG_BOMB = 1 << 0, SFLAG_COIN = 1 << 1 } SpawnFlag;

////////////////////////////////////////////////////////////
typedef enum EntityFlag {
    EFLAG_NONE = 0,
    EFLAG_ACTIVE = 1 << 0,
    EFLAG_PLAYER_CONTROLLED = 1 << 2,
    EFLAG_SPAWNER = 1 << 3,
    EFLAG_BOMB = 1 << 4,
    EFLAG_WALL = 1 << 5,
    EFLAG_EXPLOSION = 1 << 6,
    EFLAG_COIN = 1 << 7,
    EFLAG_DRAW = 1 << 8
} EntityFlag;

typedef uint32_t EntityId;

typedef struct Entity {
    /////////////////////////////////////////////////////////////////
    // Core field.
    EntityId id;
    Vector3 p;
    Vector3 pp;
    Vector3 a;
    float m;
    Quaternion rot;
    EntityFlag flags;

    // Animation.
    Texture texture;
    Rectangle bitmap_rec;
    Color tint;

    // Collision.
    Shape shape;

    /////////////////////////////////////////////////////////////////
    // Gameplay fields.
    float bomb_timer;
    float explosion_timer;

    float spawn_move_timer;
    float spawn_move_time;
    float spawn_timer;
    float spawn_time;
    float spawn_count;
    float spawn_total;
    SpawnFlag spawn_flags;

    float damage_timer;
    float damage_cooldown;

    float dash_timer;
} Entity;

typedef struct EntityEntry {
    EntityId key;
    Entity value;
} EntityEntry;

#define ENTITY_CAP 1024

typedef struct EntityManager {
    Entity entities[ENTITY_CAP];
} EntityManager;

static EntityManager EntityM = {0};

#define GetEntityManager() &EntityM

static void EntitySystemInit() { memset(&EntityM, 0, sizeof(EntityM)); }

static Entity* GetEntity(EntityId id) {
    EntityManager* em = GetEntityManager();
    Entity* entity = &em->entities[id];
    return entity;
}

static EntityId NextEntityId() {
    EntityManager* em = GetEntityManager();
    for (uint32_t i = 0; i < ArraySizeOf(em->entities); ++i) {
        Entity* entity = &em->entities[i];
        if (!TestFlag(entity->flags, EFLAG_ACTIVE)) {
            return i;
        }
    }
    Assert(false);
    return 0;
}

static Entity* AllocateEntity() {
    EntityId id = NextEntityId();
    Entity* entity = GetEntity(id);
    Assert(entity != NULL);
    memset(entity, 0, sizeof(*entity));
    entity->id = id;
    entity->flags = EFLAG_ACTIVE;
    entity->tint = WHITE;
    SetFlag(entity->flags, EFLAG_DRAW);
    return entity;
}

////////////////////////////////////////////////////////////
// Collision.

typedef struct CollisionInfo {
    Entity* entities[ENTITY_CAP];
    uint32_t len;
} CollisionInfo;

static CollisionInfo CheckEntityCollisionScene(Entity* entity) {
    Vector2 e1p = {entity->p.x, entity->p.y};
    EntityManager* em = GetEntityManager();
    CollisionInfo info = {0};
    for (uint32_t i = 0; i < ArraySizeOf(em->entities); ++i) {
        Entity* e = GetEntity(i);
        if (!TestFlag(e->flags, EFLAG_ACTIVE) || e->id == entity->id) {
            continue;
        }
        bool hit = Collides(e1p, entity->shape, (Vector2){e->p.x, e->p.y}, e->shape);
        if (hit) {
            Assert(info.len < ENTITY_CAP);
            info.entities[info.len] = e;
            ++info.len;
        }
    }
    return info;
}

////////////////////////////////////////////////////////////

static void LoadLevel() {
    char* data = LoadFileText("assets/level/level2.ldtk");
    Assert(data != NULL);
    cJSON* level = cJSON_Parse(data);
    Assert(level != NULL);
    const cJSON* defs = cJSON_GetObjectItemCaseSensitive(level, "defs");
    Assert(defs != NULL);
    const cJSON* tilesets = cJSON_GetObjectItemCaseSensitive(defs, "tilesets");
    Assert(tilesets != NULL);

    Texture* textures[256] = {0};
    const cJSON* tileset = NULL;
    cJSON_ArrayForEach(tileset, tilesets) {
        Assert(tileset != NULL);
        const cJSON* uid_js = cJSON_GetObjectItemCaseSensitive(tileset, "uid");
        Assert(uid_js != NULL && cJSON_IsNumber(uid_js));
        int uid = uid_js->valueint;

        const cJSON* path = cJSON_GetObjectItemCaseSensitive(tileset, "relPath");
        Assert(path != NULL && cJSON_IsString(path));

        char tileset_path[256] = {0};
        strcat(tileset_path, "assets/");
        strcat(tileset_path, &path->valuestring[3]);

        Texture* texture = AllocateTexture();
        *texture = LoadTexture(tileset_path);
        Assert(IsTextureReady(*texture));
        textures[uid] = texture;
    }

    const cJSON* levels = cJSON_GetObjectItemCaseSensitive(level, "levels");
    Assert(levels != NULL);
    const cJSON* cur_level = NULL;
    cJSON_ArrayForEach(cur_level, levels) {
        const cJSON* layers = cJSON_GetObjectItemCaseSensitive(cur_level, "layerInstances");
        Assert(layers != NULL);
        const cJSON* layer = NULL;
        uint32_t depth = 0;
        cJSON_ArrayForEach(layer, layers) {
            const cJSON* type = cJSON_GetObjectItemCaseSensitive(layer, "__type");
            Assert(type != NULL);

            const cJSON* width_js = cJSON_GetObjectItemCaseSensitive(layer, "__cWid");
            Assert(width_js != NULL);
            const cJSON* height_js = cJSON_GetObjectItemCaseSensitive(layer, "__cHei");
            Assert(height_js != NULL);
            int width = width_js->valueint;
            int height = height_js->valueint;

            const cJSON* x_offset_js = cJSON_GetObjectItemCaseSensitive(layer, "__pxTotalOffsetX");
            Assert(x_offset_js != NULL && cJSON_IsNumber(x_offset_js));
            const cJSON* y_offset_js = cJSON_GetObjectItemCaseSensitive(layer, "__pxTotalOffsetY");
            Assert(y_offset_js != NULL && cJSON_IsNumber(y_offset_js));
            int x_offset = x_offset_js->valueint;
            int y_offset = y_offset_js->valueint;

            const cJSON* grid_size_js = cJSON_GetObjectItemCaseSensitive(layer, "__gridSize");
            Assert(grid_size_js != NULL);
            int grid_size = grid_size_js->valueint;
            float half_grid_size = grid_size * 0.5f;

            if (strcmp(type->valuestring, "Tiles") == 0) {
                const cJSON* tileset_id = cJSON_GetObjectItemCaseSensitive(layer, "__tilesetDefUid");
                Assert(tileset_id != NULL);
                Texture* texture = textures[tileset_id->valueint];

                const cJSON* grid_tiles = cJSON_GetObjectItemCaseSensitive(layer, "gridTiles");
                Assert(grid_tiles != NULL);
                const cJSON* grid_tile = NULL;
                cJSON_ArrayForEach(grid_tile, grid_tiles) {
                    const cJSON* px_js = cJSON_GetObjectItemCaseSensitive(grid_tile, "px");
                    Assert(px_js != NULL);
                    const cJSON* src_js = cJSON_GetObjectItemCaseSensitive(grid_tile, "src");
                    Assert(src_js != NULL);

                    Entity* entity = AllocateEntity();
                    entity->p = (Vector3){px_js->child->valueint + half_grid_size,
                                          px_js->child->next->valueint + half_grid_size, depth};
                    entity->texture = *texture;
                    entity->bitmap_rec =
                        (Rectangle){src_js->child->valueint, src_js->child->next->valueint, grid_size, grid_size};
                    // FlagOff(entity->flags, EFLAG_DRAW);
                }
            } else if (strcmp(type->valuestring, "IntGrid") == 0) {
                const cJSON* grid = NULL;

                int i = 0;
                int j = 0;
                const cJSON* grids = cJSON_GetObjectItemCaseSensitive(layer, "intGridCsv");
                Assert(grids != NULL);
                cJSON_ArrayForEach(grid, grids) {
                    Assert(j <= width && i <= height);
                    int val = grid->valueint;
                    if (val == 1) {
                        Assert(val == 0 || val == 1);
                        Entity* entity = AllocateEntity();
                        FlagOff(entity->flags, EFLAG_DRAW);
                        SetFlag(entity->flags, EFLAG_WALL);
                        entity->p = (Vector3){(j * grid_size) + x_offset + half_grid_size,
                                              (i * grid_size) + y_offset + half_grid_size, depth};
                        entity->shape = (Shape){.kind = SHAPE_REC, .width = grid_size, .height = grid_size};

                        if (TestFlag(entity->flags, EFLAG_DRAW)) {
                            entity->texture = LoadTextureFromImage(GenImageColor(grid_size, grid_size, BLACK));
                            entity->bitmap_rec = (Rectangle){0, 0, grid_size, grid_size};
                        }
                    }

                    ++j;
                    i += !(j % width);
                    j %= width;
                }

            } else {
                Assert(false);
            }
            ++depth;
        }
    }

    UnloadFileText(data);
}

////////////////////////////////////////////////////////////

typedef struct GameState {
    Camera camera;

    uint16_t health;

    bool debug;
    bool game_over;

    uint32_t score;

    float timer;

    RenderTexture2D target;
} GameState;

#define RenderWidth(state) ((state)->target.texture.width)
#define RenderHeight(state) ((state)->target.texture.height)

static GameState GState;

#define GetGameState() (&GState)

static void SimpleScene(GameState* state) {
    LoadLevel();

    state->score = 0.0f;

    float ground_y = 108;

    // // Ceiling.
    // {
    //     Entity* entity = AllocateEntity();
    //     SetFlag(entity->flags, EFLAG_WALL);
    //     float y = RenderHeight(state) * 0.65;
    //     ground_y = y;
    //     int width = RenderWidth(state);
    //     int height = 120;
    //     entity->p = (Vector3){200, 30, 0};
    //     // entity->p = (Vector3){width * 0.5f, y + (height * 0.5f), 0};
    //     entity->pp = entity->p;
    //     entity->texture = LoadTextureFromImage(GenImageColor(width, height, GRAY));
    //     entity->bitmap_rec = (Rectangle){0, 0, width, height};
    //     entity->shape = (Shape){.kind = SHAPE_REC, .width = width, .height = height};
    // }

    // Player.
    {
        Entity* entity = AllocateEntity();
        SetFlag(entity->flags, EFLAG_PLAYER_CONTROLLED);
        int width = 24;
        int height = 24;
        float y = ground_y - height;
        entity->p = (Vector3){RenderWidth(state) * 0.5 + (width * 0.5f), y + (height * 0.5f), 0.0f};
        entity->pp = entity->p;
        // entity->texture = LoadTextureFromImage(GenImageColor(width, height, BLUE));
        entity->texture = GetTexture(TEXTURE_TILEMAP_CHARACTERS);
        entity->bitmap_rec = TextureGetTile(entity->texture, width, height, 1, 1, 1);
        entity->shape = (Shape){.kind = SHAPE_REC, .width = width, .height = height};
        entity->damage_cooldown = 0.4f;
        entity->damage_timer = 0.0f;
    }

    // Bomb spawner.
    {
        Entity* entity = AllocateEntity();
        SetFlag(entity->flags, EFLAG_SPAWNER);
        float y = RenderHeight(state) * 0.20;
        int width = 12;
        int height = 12;
        entity->p = (Vector3){(RenderWidth(state)) * 0.5f, y + (height * 0.5f), 0};
        entity->pp = entity->p;
        entity->texture = LoadTextureFromImage(GenImageColor(width, height, RED));
        entity->bitmap_rec = (Rectangle){0, 0, width, height};
        entity->spawn_move_time = 2.5f;
        entity->spawn_time = 0.3f;
        entity->spawn_total = 3;
        SetFlag(entity->spawn_flags, SFLAG_BOMB);
        // FlagOff(entity->flags, EFLAG_DRAW);
    }

    // Coin spawner.
    {
        Entity* entity = AllocateEntity();
        SetFlag(entity->flags, EFLAG_SPAWNER);
        int width = 50;
        int height = 25;
        float y = ground_y - height - 10.0f;
        entity->p = (Vector3){(int)(width * 0.5f), (int)(y + (height * 0.5f)), 0};
        entity->pp = entity->p;
        entity->texture = LoadTextureFromImage(GenImageColor(width, height, BLACK));
        entity->bitmap_rec = (Rectangle){0, 0, width, height};
        entity->spawn_move_time = 3.5f;
        entity->spawn_total = 1;
        SetFlag(entity->spawn_flags, SFLAG_COIN);
        FlagOff(entity->flags, EFLAG_DRAW);
    }

    state->health = 0;

    PlaySoundIfNotPlaying(SOUND_GAME_START);
}

static void GameStateInit(GameState* state) {
    memset(state, 0, sizeof(GameState));
    InitWindow(RenderWidth * 4, RenderHeight * 4, "Game");
    SetWindowState(FLAG_WINDOW_UNDECORATED);
    // ToggleFullscreen();
    SetConfigFlags(FLAG_VSYNC_HINT);
    SetTargetFPS(60);
    EntitySystemInit();
    AudioSystemInit();
    TextureCacheInit();

    state->target = LoadRenderTexture(RenderWidth, RenderHeight);

    SimpleScene(state);
}

static void GameStateUpdate(GameState* state, float delta) {
    state->timer += delta;

    if (IsSoundPlaying(GetSound(SOUND_GAME_START))) {
        return;
    }

    EntityManager* em = GetEntityManager();
    for (uint32_t i = 0; i < ArraySizeOf(em->entities); ++i) {
        Entity* entity = GetEntity(i);
        if (!TestFlag(entity->flags, EFLAG_ACTIVE)) {
            continue;
        }

        Vector3 cur_p = entity->p;
        if (TestFlag(entity->flags, EFLAG_PLAYER_CONTROLLED)) {
            entity->damage_timer += delta;

            if (state->health == 0) {
                state->game_over = true;
                FlagOff(entity->flags, EFLAG_ACTIVE);
                PlaySoundIfNotPlaying(SOUND_GAME_OVER);
                continue;
            }

            bool moved = false;
            Vector3 direction = Vector3Zero();
            if (IsKeyDown(KEY_RIGHT)) {
                direction = Vector3Add(direction, (Vector3){1, 0, 0});
                moved = true;
            }
            if (IsKeyDown(KEY_LEFT)) {
                direction = Vector3Add(direction, (Vector3){-1, 0, 0});
                moved = true;
            }

            float speed = 2000.0f;
            entity->dash_timer += delta;
            if (IsKeyPressed(KEY_SPACE)) {
                entity->dash_timer = 0.0f;
                speed *= 30.0f;
            }
            Vector3 a = Vector3Scale(direction, speed);
            Vector3 v = Vector3Subtract(entity->p, entity->pp);
            if (fabs(v.x) > 10.0f) {
                v.x = (v.x < 0.0f ? -1.0f : 1.0f) * 10.0f;
            }
            Vector3 displacement = Vector3Add(v, Vector3Scale(a, delta * delta));
            if (!moved) {
                displacement = Vector3Scale(displacement, 0.6f);
            }

            entity->p = Vector3Add(entity->p, displacement);

            bool walking = !Vector3Equals(displacement, Vector3Zero());
            CollisionInfo info = CheckEntityCollisionScene(entity);
            for (uint32_t i = 0; i < info.len; ++i) {
                Entity* e = info.entities[i];
                if (TestFlag(e->flags, EFLAG_EXPLOSION) && entity->damage_timer >= entity->damage_cooldown) {
                    entity->damage_timer = 0.0f;
                    if (state->health > 0) {
                        --state->health;
                    }
                    walking = false;
                }
                if (TestFlag(e->flags, EFLAG_WALL)) {
                    // Project out in x dir.
                    float dist_x = entity->p.x - e->p.x;
                    float delta_x = (0.5f * (entity->shape.width + e->shape.width)) - fabs(dist_x);
                    Assert(delta_x >= 0.0f);
                    float dir_x = dist_x / fabs(dist_x);
                    float disp_x = dir_x * delta_x;
                    entity->p.x += disp_x;
                    cur_p = entity->p;
                    walking = false;
                }
                if (TestFlag(e->flags, EFLAG_COIN)) {
                    FlagOff(e->flags, EFLAG_ACTIVE);
                    ++state->score;
                    PlaySoundIfNotPlaying(SOUND_PICKUP_COIN);
                }
            }

            if (walking) {
                static const SoundId sounds[2] = {SOUND_FOOTSTEP_LEFT, SOUND_FOOTSTEP_RIGHT};
                static uint16_t sound_idx = 0;

                Sound walk_sound = GetSound(sounds[sound_idx]);
                if (!IsSoundPlaying(walk_sound)) {
                    sound_idx = (sound_idx + 1) % ArraySizeOf(sounds);
                    PlaySound(GetSound(sounds[sound_idx]));
                }
            }
        }

        if (TestFlag(entity->flags, EFLAG_SPAWNER)) {
            entity->spawn_move_timer += delta;
            entity->spawn_timer += delta;

            if ((entity->spawn_timer >= entity->spawn_time) && entity->spawn_count > 0) {
                --entity->spawn_count;
                entity->spawn_timer = 0.0f;
                // Calculate random initial velocity.
                Vector2 v = Vector2Rotate((Vector2){1, 0}, GetRandomValue(0, 180) * DEG2RAD);
                v = Vector2Scale(v, GetRandomValue(10, 100));

                // Spawn bomb.
                if (TestFlag(entity->spawn_flags, SFLAG_BOMB)) {
                    static bool RIGHT = true;

                    Entity* bomb = AllocateEntity();
                    SetFlag(bomb->flags, EFLAG_BOMB);
                    int width = 4;
                    int height = 6;
                    bomb->p = (Vector3){50, 200, 0};
                    bomb->p = entity->p;
                    float x_displacement = GetRandomValue(200, 400);
                    x_displacement *= RIGHT ? 1.0f : -1.0f;
                    RIGHT = !RIGHT;
                    Vector3 displacement = Vector3Scale((Vector3){x_displacement, -1600, 0}, delta);
                    bomb->pp = Vector3Subtract(bomb->p, displacement);
                    bomb->texture = LoadTextureFromImage(GenImageColor(width, height, WHITE));
                    bomb->bitmap_rec = (Rectangle){0, 0, width, height};
                    bomb->shape = (Shape){.kind = SHAPE_REC, .width = width, .height = height};
                    bomb->m = 3.0f;
                }

                // Spawn Coin.
                if (TestFlag(entity->spawn_flags, SFLAG_COIN)) {
                    Entity* coin = AllocateEntity();
                    SetFlag(coin->flags, EFLAG_COIN);
                    coin->p = entity->p;
                    int width = 12;
                    int height = 12;
                    coin->texture = GetTexture(TEXTURE_TILEMAP_BACKGROUND);
                    coin->bitmap_rec = TextureGetTile(coin->texture, width, height, 0, 0, 45);
                    coin->shape = (Shape){.kind = SHAPE_REC, .width = width, .height = height};
                }
            }

            if (entity->spawn_move_timer >= entity->spawn_move_time) {
                entity->spawn_move_timer = 0.0f;
                entity->spawn_count = entity->spawn_total;
                // Move randomly.
                {
                    float min = 0.0f;
                    float max = RenderWidth(state);
                    float choice = GetRandomValue(min, max);
                    entity->p.x = choice;
                }
            }
        }

        if (TestFlag(entity->flags, EFLAG_BOMB)) {
            entity->bomb_timer += delta;
            if (entity->bomb_timer >= 4.0f) {
                // Spawn explosion.
                {
                    Entity* explosion = AllocateEntity();
                    SetFlag(explosion->flags, EFLAG_EXPLOSION);
                    explosion->p = entity->p;
                    explosion->pp = explosion->p;
                    int width = 100;
                    int height = 100;
                    Image image = GenImageColor(width, height, BLANK);
                    Color color = YELLOW;
                    color.a = 125;
                    ImageDrawCircle(&image, width * 0.5f, height * 0.5f, width * 0.5f, color);
                    explosion->texture = LoadTextureFromImage(image);
                    explosion->bitmap_rec = (Rectangle){0, 0, width, height};
                    explosion->shape = (Shape){.kind = SHAPE_CIRCLE, .radius = width * 0.5f};
                }
                FlagOff(entity->flags, EFLAG_ACTIVE);
                continue;
            }

            // TODO: Once camera is set up, start in camera space and convert to screen space.
            const Vector3 g = {0, 9.8 * 1000.0f, 0};
            Vector3 force = Vector3Scale(g, entity->m);
            entity->a = Vector3Scale(force, 1.0f / entity->m);
            Vector3 displacement =
                Vector3Add(Vector3Subtract(entity->p, entity->pp), Vector3Scale(entity->a, delta * delta));
            entity->p = Vector3Add(entity->p, displacement);

            CollisionInfo info = CheckEntityCollisionScene(entity);
            for (uint32_t i = 0; i < info.len; ++i) {
                Entity* e = info.entities[i];
                bool still_hit =
                    Collides((Vector2){entity->p.x, entity->p.y}, entity->shape, (Vector2){e->p.x, e->p.y}, e->shape);
                if (TestFlag(e->flags, EFLAG_WALL) && still_hit) {
                    // TODO: IMPROVEMENTS:
                    // * Run the constraint solve multiple times to resolve multiple constraints. the
                    // bombs sometimes will stay a frame below collisions before teloporting back up. This
                    // is because of the still_hits check. The bomb can fall below first level collision boxes
                    // then get pushed up by second layer and then next frame gets pushed up again.
                    // * Need to deal with bombs falling down getting pushed right instead of up. They will just
                    // oscillate infinitely back and forth. Idea 1) Subdivide the delta time and run phsyics sim
                    // at higher Hz than render Hz (maybe like double) - this will decrease the amount of distance
                    // entities will travel each physics frame. Idea 2) Have special logic something along the lines of
                    // if the difference between pen_up and pen_left is "close enough" project along the direction the
                    // closest to the incoming impact (does this even make sense what if the bomb hits a corner
                    // perfectly)?
                    Vector3 velocity = Vector3Subtract(entity->p, entity->pp);
                    Vector3 distance = Vector3Subtract(entity->p, e->p);
                    float pen_x = (0.5f * (entity->shape.width + e->shape.width)) - fabs(distance.x);
                    Assert(pen_x >= 0.0f);
                    float pen_y = (0.5f * (entity->shape.height + e->shape.height)) - fabs(distance.y);
                    Assert(pen_y >= 0.0f);
                    Vector3 bounce_and_friction = {0.0f};
                    if (pen_x <= pen_y) {
                        pen_y = 0.0f;
                        bounce_and_friction = (Vector3){-0.2f, 0.2f, 1.0f};
                    } else {
                        pen_x = 0.0f;
                        bounce_and_friction = (Vector3){0.2f, -0.2f, 1.0f};
                    }
                    Vector3 penetration = {pen_x, pen_y, 0.0f};

                    // Get the direction of the displacement, by normalizing the y direction.
                    Vector3 direction =
                        Vector3Multiply(distance, (Vector3){1.0f / fabs(distance.x), 1.0f / fabs(distance.y), 0.0f});
                    Vector3 displacement = Vector3Multiply(direction, penetration);
                    // Vector3 foo = entity->p;
                    entity->p = Vector3Add(entity->p, displacement);

                    cur_p = Vector3Subtract(entity->p, Vector3Multiply(bounce_and_friction, velocity));
                    entity->a = Vector3Zero();
                    Assert(fabs(entity->p.x - e->p.x) >= (0.5f * (entity->shape.width + e->shape.width)) ||
                           fabs(entity->p.y - e->p.y) >= (0.5f * (entity->shape.height + e->shape.height)));
                }
            }
        }

        if (TestFlag(entity->flags, EFLAG_EXPLOSION)) {
            static const float TIME = 0.2f;
            entity->explosion_timer += delta;
            if (entity->explosion_timer >= TIME) {
                FlagOff(entity->flags, EFLAG_ACTIVE);
                continue;
            }

            // Explosion shading.
            Color tint = {Lerp(BLACK.r, WHITE.r, entity->explosion_timer / TIME),
                          Lerp(BLACK.g, WHITE.g, entity->explosion_timer / TIME),
                          Lerp(BLACK.b, WHITE.b, entity->explosion_timer / TIME), 255};
            entity->tint = tint;

            PlaySoundIfNotPlaying(SOUND_BOMB_EXPLOSION);
        }

        entity->pp = cur_p;
    }
}

static void GameStateDraw(GameState* state, float delta) {
    Entity* entities[ENTITY_CAP] = {0};
    uint32_t size = 0;
    EntityManager* em = GetEntityManager();
    for (uint32_t i = 0; i < ArraySizeOf(em->entities); ++i) {
        Entity* entity = GetEntity(i);
        if (!TestFlag(entity->flags, EFLAG_ACTIVE) || !TestFlag(entity->flags, EFLAG_DRAW)) {
            continue;
        }
        entities[size] = entity;
        ++size;
    }

    // TODO: Should we do something smarter here than a bubble sort?
    for (uint32_t i = 0; i < size; ++i) {
        Entity* e1 = entities[i];
        for (uint32_t j = i + 1; j < size; ++j) {
            Entity* e2 = entities[j];
            if (e2->p.z > e1->p.z) {
                entities[i] = entities[j];
                entities[j] = e1;
                e1 = e2;
            }
        }
    }

    BeginTextureMode(state->target);

    ClearBackground(WHITE);

    state->debug ^= (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_D));
    if (state->debug) {
        DrawFPS(5, 5);
    }

    for (uint32_t i = 0; i < size; ++i) {
        Entity* entity = entities[i];
        Color tint = entity->tint;
        if (entity->damage_timer < entity->damage_cooldown) {
            tint.a = Lerp(tint.a, 0, sin(state->timer * 100.0f));
        }

        float half_width = entity->bitmap_rec.width * 0.5f;
        float half_height = entity->bitmap_rec.height * 0.5f;
        DrawTextureRec(entity->texture, entity->bitmap_rec,
                       (Vector2){entity->p.x - half_width, entity->p.y - half_height}, tint);
    }

    // Draw UI.
    {
        // Play UI.
        {
            Font font = GetFontDefault();
            float fontsize = 12.0f;
            float spacing = fontsize / font.baseSize;
            Color color = WHITE;

            Vector2 pos = {0.0f};
            pos.x = RenderWidth(state) * 0.92;

            // Health text.
            {
                char text[256] = {0};
                memset(text, '*', sizeof(char) * state->health);
                Vector2 size = MeasureTextEx(font, text, fontsize, spacing);
                pos.y += 5.0f;
                Vector2 p = {pos.x - size.x, pos.y};
                DrawTextEx(font, text, p, fontsize, spacing, color);
                pos.y += size.y;
            }

            // Score text.
            {
                char text[256] = {0};
                sprintf(text, "%d", state->score);
                Vector2 size = MeasureTextEx(font, text, fontsize, spacing);
                pos.y += 2.0f;
                Vector2 p = {pos.x - size.x, pos.y};
                DrawTextEx(font, text, p, fontsize, spacing, color);
            }
        }

        if (state->game_over) {
            static const char* GAME_OVER_TEXT = "Game Over";
            static const char* RETRY_TEXT = "Press Enter to Retry";

            Color bg_color = {BLACK.r, BLACK.g, BLACK.b, BLACK.a * 0.4f};

            Font font = GetFontDefault();
            float fontsize = 24.0f;
            float spacing = fontsize / font.baseSize;
            Color text_color = WHITE;
            {
                DrawRectangle(0, 0, RenderWidth(state), RenderHeight(state), bg_color);

                Vector2 size = MeasureTextEx(font, GAME_OVER_TEXT, fontsize, spacing);
                float y = (RenderHeight(state) - size.y) * 0.5f;
                DrawText(GAME_OVER_TEXT, (RenderWidth(state) - size.x) * 0.5f, y, fontsize, text_color);
                y += size.y + 5.0f;
                fontsize = 18.0f;
                size = MeasureTextEx(font, RETRY_TEXT, fontsize, spacing);
                DrawText(RETRY_TEXT, (RenderWidth(state) - size.x) * 0.5f, y, fontsize, text_color);
            }

            if (IsKeyDown(KEY_ENTER)) {
                state->game_over = false;
                // TODO: Should probably move this somewhere else.
                SimpleScene(state);
            }
        }
    }

    EndTextureMode();

    // Draw render target to screen and optionally upscale.
    float scale =
        fmin(((float)GetScreenWidth()) / RenderWidth(state), ((float)GetScreenHeight()) / RenderHeight(state));
    BeginDrawing();
    ClearBackground(WHITE);
    Texture texture = state->target.texture;
    DrawTexturePro(texture, (Rectangle){0, 0, texture.width, -texture.height},
                   (Rectangle){0, 0, texture.width * scale, texture.height * scale}, (Vector2){0.0f, 0.0f}, 0.0f,
                   WHITE);
    EndDrawing();
}

////////////////////////////////////////////////////////////

int main(void) {
    GameStateInit(&GState);

    while (!WindowShouldClose()) {
        float delta = GetFrameTime();
        GameStateUpdate(&GState, delta);
        GameStateDraw(&GState, delta);
    }

    CloseWindow();

    return 0;
}