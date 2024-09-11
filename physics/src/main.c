#ifndef UNITY_BUILD
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "raylib.h"
#include "raymath.h"

#endif

////////////////////////////////////////////////////////////
// Core types.

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float f32;
typedef double f64;

////////////////////////////////////////////////////////////
// Math

#define sqrtf32 sqrtf

typedef struct Vec2 {
    f32 x;
    f32 y;
} Vec2;

#define Vec2Lit(a, b) ((Vec2){.x = (a), .y = (b)})

static Vec2 Vec2Zero() {
    static Vec2 v = Vec2Lit(0.0f, 0.0f);
    return v;
}

static Vec2 Vec2Add(Vec2 v1, Vec2 v2) { return Vec2Lit(v1.x + v2.x, v1.y + v2.y); }
static Vec2 Vec2Sub(Vec2 v1, Vec2 v2) { return Vec2Lit(v1.x - v2.x, v1.y - v2.y); }
static f32 Vec2Len(Vec2 v) { return sqrtf32((v.x * v.x) + (v.y * v.y)); }
static f32 Vec2Dot(Vec2 v1, Vec2 v2) { return (v1.x * v2.x) + (v1.y * v2.y); }

typedef struct Vec3 {
    f32 x;
    f32 y;
    f32 z;
} Vec3;

#define Vec3Lit(a, b, c) ((Vec3){.x = (a), .y = (b), .z = (c)})
#define Vec3ToRVec3(v) ((Vector3){.x = (v).x, .y = (v).y, .z = (v.z)})

static Vec3 Vec3Zero() {
    static Vec3 v = Vec3Lit(0.0f, 0.0f, 0.0f);
    return v;
}

static Vec3 Vec3Add(Vec3 v1, Vec3 v2) { return Vec3Lit(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z); }
static Vec3 Vec3Sub(Vec3 v1, Vec3 v2) { return Vec3Lit(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z); }
static f32 Vec3Len(Vec3 v) { return sqrtf32((v.x * v.x) + (v.y * v.y) + (v.z * v.z)); }
static f32 Vec3Dot(Vec3 v1, Vec3 v2) { return (v1.x * v2.x) + (v1.y * v2.y) + (v1.z * v2.z); }

////////////////////////////////////////////////////////////

const float RenderWidth = 1280;
const float RenderHeight = 720;

static const Vector3 FLIP_Y = {1, -1, 1};
static Vector3 G = {0.0f, -15.0f, 0.0f};

static const Vector3 ORIGIN = {0.0f, 0.0f, 0.0f};
static const Vector2 ORIGIN2D = {ORIGIN.x, ORIGIN.y};

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
    EFLAG_PROJECTILE = 1 << 7,
    EFLAG_PARTICLE = 1 << 8
} EntityFlag;

typedef uint32_t EntityId;

typedef struct Entity {
    /////////////////////////////////////////////////////////////////
    // Core field.
    EntityId id;
    Vec3 p;
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
    FlagOn(entity->flags, EFLAG_DRAW);
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

typedef enum InputDir {
    DIR_NONE = 0,
    DIR_UP,
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT,

    DIR_COUNT
} InputDir;

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

    Entity* player;

    struct {
        Entity* head;
        Entity* tail;
    } monsters;
} GameState;

#define RenderWidth(state) ((state)->target.texture.width)
#define RenderHeight(state) ((state)->target.texture.height)

static GameState GState;

#define GetGameState() (&GState)

static void SimpleScene(GameState* state) {
    // LoadLevel();

    state->score = 0.0f;
    state->health = 3;

    // Player.
    {
        Entity* entity = AllocateEntity();
        FlagOn(entity->flags, EFLAG_PLAYER_CONTROLLED);
        int width = 24 * 3;
        int height = 24 * 3;
        entity->p = Vec3Zero();

        Image image = GenImageColor(width, height, RED);
        entity->texture = LoadTextureFromImage(image);
        entity->bitmap_rec = (Rectangle){0, 0, width, height};
        entity->shape = (Shape){.kind = SHAPE_REC, .width = width, .height = height};

        state->player = entity;
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

        // Update animation timer if it is a looping animation.
        entity->animation_timer += dt * entity->animation_loops;
    }

    state->debug ^= (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_ONE));
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
        Vector3 p = Vec3ToRVec3(entity->p);
        DrawTextureRec(entity->texture, src, (Vector2){p.x - half_width, p.y - half_height}, tint);
        if (false) {
            DrawCircle(p.x, p.y, 1, BLACK);
        }
    }

    EndMode2D();

    if (state->debug) {
        DrawFPS(5, 5);
    }

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