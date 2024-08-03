#ifndef UNITY_BUILD
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "raylib.h"
#include "raymath.h"

#endif

const float RenderWidth = 1280;
const float RenderHeight = 720;

static const Vector3 FLIP_Y = {1, -1, 1};

static const Vector3 ORIGIN = {0.0f, 0.0f, 0.0f};
static const Vector2 ORIGIN2D = {ORIGIN.x, ORIGIN.y};
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
    SOUND_TYPING_GOOD,
    SOUND_TYPING_BAD,
    SOUND_TYPING_DELETE,
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

    Sounds[SOUND_TYPING_GOOD] = LoadSound("assets/sound/typing_good.wav");
    Assert(IsSoundReady(Sounds[SOUND_TYPING_GOOD]));

    Sounds[SOUND_TYPING_BAD] = LoadSound("assets/sound/typing_bad.wav");
    Assert(IsSoundReady(Sounds[SOUND_TYPING_BAD]));

    Sounds[SOUND_TYPING_DELETE] = LoadSound("assets/sound/typing_delete.wav");
    Assert(IsSoundReady(Sounds[SOUND_TYPING_DELETE]));
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

#define ANIMATION_FRAME_CAP (4)

////////////////////////////////////////////////////////////
typedef enum EntityFlag {
    EFLAG_NONE = 0,
    EFLAG_ACTIVE = 1 << 0,
    EFLAG_DRAW = 1 << 1,
    EFLAG_PLAYER_CONTROLLED = 1 << 2,
    EFLAG_SPAWNER = 1 << 3,
    EFLAG_WALL = 1 << 4,
    EFLAG_MONSTER = 1 << 5,
    EFLAG_UNSPAWNED = 1 << 6,
} EntityFlag;

typedef uint32_t EntityId;

typedef struct Entity {
    /////////////////////////////////////////////////////////////////
    // Core field.
    EntityId id;
    Vector3 p;
    Vector3 pp;
    Vector3 v;
    Vector3 a;
    float m;
    Quaternion rot;
    EntityFlag flags;
    // TODO: Definitely reconsider this vs a better approach for orientation.
    bool right;

    // Animation.
    Texture texture;
    Rectangle bitmap_rec;
    uint32_t animation_frame_count;
    float animation_frame_time;
    float animation_timer;
    bool animation_loops;

    Color tint;

    // Collision.
    Shape shape;

    /////////////////////////////////////////////////////////////////
    // Gameplay fields.
    bool dead;

    struct {
        struct Entity* prev;
        struct Entity* next;
    } monster_link;

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
    entity->animation_frame_count = 1;    // Avoid division by zero in animation system.
    entity->animation_frame_time = 1.0f;  // Avoid division by zero in animation system.
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

            } else if (strcmp(type->valuestring, "Entities") == 0) {
                const cJSON* entities = cJSON_GetObjectItemCaseSensitive(layer, "entityInstances");
                Assert(entities != NULL);
                const cJSON* entity = NULL;
                cJSON_ArrayForEach(entity, entities) {
                    // TODO: Implement entity loading later. Seems a bit too much work for what could be done
                    // with harcoding.
                    // const cJSON* identifier_js = cJSON_GetObjectItemCaseSensitive(entity, "__identifier");
                    // Assert(identifier_js != NULL);

                    // // Position.
                    // const cJSON* x_world_js = cJSON_GetObjectItemCaseSensitive(entity, "__worldX");
                    // Assert(x_world_js != NULL && cJSON_IsNumber(x_world_js));
                    // const cJSON* y_world_js = cJSON_GetObjectItemCaseSensitive(entity, "__worldY");
                    // Assert(y_world_js != NULL && cJSON_IsNumber(y_world_js));
                    // int x_world = x_world_js->valueint;
                    // int y_world = y_world_js->valueint;

                    // // Size.
                    // const cJSON* entity_width_js = cJSON_GetObjectItemCaseSensitive(entity, "width");
                    // Assert(entity_width_js != NULL && cJSON_IsNumber(entity_width_js));
                    // const cJSON* entity_height_js = cJSON_GetObjectItemCaseSensitive(entity, "height");
                    // Assert(entity_height_js != NULL && cJSON_IsNumber(entity_height_js));
                    // int entity_width = entity_width_js->valueint;
                    // int entity_height = entity_height_js->valueint;

                    // // Sprite.
                    // const cJSON* tile = cJSON_GetObjectItemCaseSensitive(entity, "__tile");
                    // Assert(tile != NULL);
                    // const cJSON* tile_id = cJSON_GetObjectItemCaseSensitive(tile, "tilesetUid");
                    // Assert(tile_id != NULL);
                    // const cJSON* tile_x = cJSON_GetObjectItemCaseSensitive(tile, "x");
                    // Assert(tile_x != NULL);
                    // const cJSON* tile_y = cJSON_GetObjectItemCaseSensitive(tile, "y");
                    // Assert(tile_y != NULL);
                    // const cJSON* tile_w = cJSON_GetObjectItemCaseSensitive(tile, "w");
                    // Assert(tile_w != NULL);
                    // const cJSON* tile_h = cJSON_GetObjectItemCaseSensitive(tile, "h");
                    // Assert(tile_h != NULL);

                    // Entity* entity = AllocateEntity();
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
    /////////////////////////////////////////
    // Core fields
    Camera2D camera;

    uint16_t health;

    bool debug;
    bool game_over;

    uint32_t score;

    float timer;

    RenderTexture2D target;

    /////////////////////////////////////////
    // Gameplay fields
    Entity* focus;

    struct {
        Entity* head;
        Entity* tail;
    } monsters;
} GameState;

#define RenderWidth(state) ((state)->target.texture.width)
#define RenderHeight(state) ((state)->target.texture.height)

static GameState GState;

#define GetGameState() (&GState)

static void SpawnMonster(GameState* state) {
    // Monster.
    {
        Entity* entity = AllocateEntity();
        SetFlag(entity->flags, EFLAG_MONSTER);
        int width = 12;
        int height = 12;
        float y = (0.5f * RenderHeight(state)) + GetRandomValue(-10, 10);
        entity->p = (Vector3){RenderWidth(state) * 0.8 + (width * 0.5f), y + (height * 0.5f), 0.0f};
        entity->pp = entity->p;
        entity->texture = GetTexture(TEXTURE_TILEMAP_BACKGROUND);
        entity->bitmap_rec = TextureGetTile(entity->texture, width, height, 0, 0, 3);
        entity->animation_frame_count = 4;
        entity->animation_frame_time = 0.2f;
        entity->shape = (Shape){.kind = SHAPE_REC, .width = width, .height = height};

        if (state->focus == NULL) {
            state->focus = entity;
        }

        if (state->monsters.head == NULL) {
            state->monsters.head = entity;
            state->monsters.tail = entity;
        } else {
            state->monsters.tail->monster_link.next = entity;
            entity->monster_link.prev = state->monsters.tail;
            state->monsters.tail = entity;
        }
    }
}

static void SimpleScene(GameState* state) {
    // LoadLevel();

    SpawnMonster(state);

    state->score = 0.0f;
    state->health = 3;

    // Player.
    {
        Entity* entity = AllocateEntity();
        SetFlag(entity->flags, EFLAG_PLAYER_CONTROLLED);
        int width = 24 * 3;
        int height = 24 * 3;
        entity->p = ORIGIN;
        entity->pp = entity->p;

        Image image = GenImageColor(width, height, RED);
        entity->texture = LoadTextureFromImage(image);
        entity->bitmap_rec = (Rectangle){0, 0, width, height};
        entity->shape = (Shape){.kind = SHAPE_REC, .width = width, .height = height};
    }
}

static void GameStateInit(GameState* state) {
    memset(state, 0, sizeof(GameState));
    InitWindow(RenderWidth, RenderHeight, "Game");
    SetWindowState(FLAG_WINDOW_UNDECORATED);
    // ToggleFullscreen();
    SetConfigFlags(FLAG_VSYNC_HINT);
    SetTargetFPS(60);
    EntitySystemInit();
    AudioSystemInit();
    TextureCacheInit();

    state->target = LoadRenderTexture(RenderWidth, RenderHeight);

    SimpleScene(state);

    {
        state->camera.target = ORIGIN2D;
        state->camera.offset = (Vector2){GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};
        state->camera.rotation = 0.0f;  // in deg
        state->camera.zoom = 1.0f;
    }
}

static void GameStateUpdate(GameState* state, float dt) {
    state->timer += dt;

    EntityManager* em = GetEntityManager();
    for (uint32_t i = 0; i < ArraySizeOf(em->entities); ++i) {
        Entity* entity = GetEntity(i);
        if (!TestFlag(entity->flags, EFLAG_ACTIVE)) {
            continue;
        }

        Vector3 cur_p = entity->p;
        if (TestFlag(entity->flags, EFLAG_PLAYER_CONTROLLED)) {
            static const float MAX_SPEED = 620.0f;
            static const float ACCELERATION = MAX_SPEED * 2.0f;
            static float ENABLE_FRICTION_TIMER = 0.0f;
            static const float ENABLE_FRICTION_TIME_LIMIT = 0.08f;
            static bool KEY_WAS_DOWN[KEY_UP - KEY_RIGHT + 1] = {0};
            static int KEY_OPPOSITE[KEY_UP - KEY_RIGHT + 1] = {KEY_DOWN, KEY_UP, KEY_RIGHT, KEY_LEFT};
            static bool DIR_SIGNALS[KEY_UP - KEY_RIGHT + 1] = {0};

            // Determine move direction signal for keyboard. (i.e. if we press left and then press right, we
            // should then move right instead of stop.)
            for (int key = KEY_RIGHT; key <= KEY_UP; ++key) {
                int key_offset = KEY_UP - key;
                int key_opposite_offset = KEY_UP - KEY_OPPOSITE[key_offset];
                if (IsKeyDown(key)) {
                    if (!KEY_WAS_DOWN[key_offset]) {
                        KEY_WAS_DOWN[key_offset] = true;
                        DIR_SIGNALS[key_offset] = true;
                        DIR_SIGNALS[key_opposite_offset] = false;
                    }
                } else {
                    KEY_WAS_DOWN[key_offset] = false;
                    DIR_SIGNALS[key_offset] = false;
                    // If we were holding down the opposite direction, move that way now.
                    DIR_SIGNALS[key_opposite_offset] = KEY_WAS_DOWN[key_opposite_offset];
                }
            }

            Vector3 wish_dir = Vector3Zero();
            bool moved = false;
            if (DIR_SIGNALS[KEY_UP - KEY_UP]) {
                moved = true;
                wish_dir = Vector3Add(wish_dir, Vector3Multiply(FLIP_Y, (Vector3){0.0f, 1.0f, 0.0f}));
            } else if (DIR_SIGNALS[KEY_UP - KEY_DOWN]) {
                moved = true;
                wish_dir = Vector3Add(wish_dir, Vector3Multiply(FLIP_Y, (Vector3){0.0f, -1.0f, 0.0f}));
            }

            if (DIR_SIGNALS[KEY_UP - KEY_RIGHT]) {
                moved = true;
                wish_dir = Vector3Add(wish_dir, Vector3Multiply(FLIP_Y, (Vector3){1.0f, 0.0f, 0.0f}));
            } else if (DIR_SIGNALS[KEY_UP - KEY_LEFT]) {
                moved = true;
                wish_dir = Vector3Add(wish_dir, Vector3Multiply(FLIP_Y, (Vector3){-1.0f, 0.0f, 0.0f}));
            }
            wish_dir = Vector3Normalize(wish_dir);

            if (moved) {
                ENABLE_FRICTION_TIMER = 0.0f;
            } else {
                ENABLE_FRICTION_TIMER += dt;
            }

            // friction.
            {
                float speed = Vector3Length(entity->v);
                if (speed > 0.0f && ENABLE_FRICTION_TIMER > ENABLE_FRICTION_TIME_LIMIT) {
                    TraceLog(LOG_INFO, "friction");
                    float friction = 1.0f;
                    if (!moved) {
                        friction *= 40.0f;
                    }
                    float drop = (ACCELERATION * 0.1f) * friction * dt;
                    float new_speed = speed - drop;
                    if (new_speed < 0.0f) {
                        new_speed = 0.0f;
                    }
                    new_speed /= speed;
                    entity->v = Vector3Scale(entity->v, new_speed);
                }
            }

            // move.
            {
                Vector3 new_dir = Vector3Normalize(entity->v);
                float new_speed = Vector3Length(entity->v);
                if (moved) {
                    // static const float TURN_SPEED = 20.0f;
                    // Vector3 dir_angle = Vector3Angle(new_dir, wish_dir);

                    new_dir = wish_dir;
                    new_speed += ACCELERATION * dt;
                }

                new_speed = Clamp(new_speed, 0.0f, MAX_SPEED);
                entity->v = Vector3Scale(new_dir, new_speed);
                // TraceLog(LOG_INFO, "v %f", Vector3Length(entity->v));

                entity->p = Vector3Add(entity->p, Vector3Scale(entity->v, dt));
            }
        }

        if (TestFlag(entity->flags, EFLAG_MONSTER)) {
        }

        entity->pp = cur_p;

        // Update animation timer if it is a looping animation.
        entity->animation_timer += dt * entity->animation_loops;
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

    // BeginTextureMode(state->target);
    BeginDrawing();

    ClearBackground(WHITE);
    BeginMode2D(state->camera);

    for (uint32_t i = 0; i < size; ++i) {
        Entity* entity = entities[i];
        Color tint = entity->tint;

        if (state->focus != NULL && entity == state->focus) {
            tint = YELLOW;
            // tint.a = 0.4f;
        }

        float half_width = entity->bitmap_rec.width * 0.5f;
        float half_height = entity->bitmap_rec.height * 0.5f;
        // Compute animation rec.
        float animation_time = entity->animation_frame_count * entity->animation_frame_time;
        uint32_t animation_frame_idx = fmod(entity->animation_timer, animation_time) / entity->animation_frame_time;
        Assert(entity->animation_frame_time != 0.0f);
        Rectangle src = {
            entity->bitmap_rec.x + (animation_frame_idx * entity->bitmap_rec.width),
            entity->bitmap_rec.y,
            entity->bitmap_rec.width * (entity->right ? 1.0f : -1.0f),
            entity->bitmap_rec.height,

        };
        DrawTextureRec(entity->texture, src, (Vector2){entity->p.x - half_width, entity->p.y - half_height}, tint);
        if (false) {
            DrawCircle(entity->p.x, entity->p.y, 1, BLACK);
        }
    }

    EndMode2D();

    // Draw UI.
    {
        // Play UI.
        {
            Font font = GetFontDefault();
            float fontsize = 12.0f;
            float spacing = fontsize / font.baseSize;
            Color color = WHITE;

            {
                float x = RenderWidth(state) * 0.92;
                float y = 0.0f;

                // Health text.
                {
                    char text[256] = {0};
                    memset(text, '*', sizeof(char) * state->health);
                    Vector2 size = MeasureTextEx(font, text, fontsize, spacing);
                    y += 5.0f;
                    Vector2 pos = {x - size.x, y};
                    DrawTextEx(font, text, pos, fontsize, spacing, color);
                    y += size.y;
                }

                // Score text.
                {
                    char text[256] = {0};
                    sprintf(text, "%d", state->score);
                    Vector2 size = MeasureTextEx(font, text, fontsize, spacing);
                    y += 2.0f;
                    Vector2 pos = {x - size.x, y};
                    DrawTextEx(font, text, pos, fontsize, spacing, color);
                }
            }
        }

        if (state->game_over) {
            static const char* GAME_OVER_TEXT = "Game Over";
            static const char* RETRY_TEXT = "Press Enter to Retry";

            Color bg_color = {GRAY.r, GRAY.g, GRAY.b, GRAY.a * 0.4f};

            Font font = GetFontDefault();
            float fontsize = 24.0f;
            float spacing = fontsize / font.baseSize;
            Color text_color = WHITE;
            {
                DrawRectangle(0, 0, RenderWidth(state), RenderHeight(state), bg_color);

                float x = RenderWidth(state) * 0.5f;
                float y = RenderHeight(state) * 0.3f;

                {
                    Vector2 size = MeasureTextEx(font, GAME_OVER_TEXT, fontsize, spacing);
                    Vector2 pos = {x - (size.x * 0.5f), y - (size.y * 0.5f)};
                    DrawText(GAME_OVER_TEXT, pos.x, pos.y, fontsize, text_color);
                    y += size.y;
                }
                {
                    Vector2 size = MeasureTextEx(font, RETRY_TEXT, fontsize, spacing);
                    y += 5.0f;
                    Vector2 pos = {x - (size.x * 0.5f), y - (size.y * 0.5f)};
                    DrawText(RETRY_TEXT, pos.x, pos.y, fontsize, text_color);
                    y += size.y;
                }
            }

            if (IsKeyDown(KEY_ENTER)) {
                state->game_over = false;
                // TODO: Should probably move this somewhere else.
                SimpleScene(state);
            }
        }
    }

    EndDrawing();
    // EndTextureMode();

    // // Draw render target to screen and optionally upscale.
    // float scale =
    //     fmin(((float)GetScreenWidth()) / RenderWidth(state), ((float)GetScreenHeight()) / RenderHeight(state));
    // BeginDrawing();
    // ClearBackground(WHITE);
    // Texture texture = state->target.texture;
    // DrawTexturePro(texture, (Rectangle){0, 0, texture.width, -texture.height},
    //                (Rectangle){0, 0, texture.width * scale, texture.height * scale}, (Vector2){0.0f, 0.0f}, 0.0f,
    //                WHITE);

    // state->debug ^= (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_D));
    // if (state->debug) {
    //     DrawFPS(5, 5);
    // }
    // EndDrawing();
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