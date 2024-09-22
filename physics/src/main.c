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
// S:Math

#define f32abs fabs
#define f32min fminf
#define f32max fmaxf
#define sqrtf32 sqrtf
#define fhalf(x) ((x) * 0.5f)

typedef struct Vec2 {
    f32 x;
    f32 y;
} Vec2;

#define Vec2Lit(a, b) ((Vec2){.x = (a), .y = (b)})
#define Vec2ToRVector2(v) ((Vector2){.x = (v).x, .y = (v).y})

static Vec2 Vec2Zero() {
    static Vec2 v = Vec2Lit(0.0f, 0.0f);
    return v;
}

static Vec2 Vec2Add(Vec2 v1, Vec2 v2) { return Vec2Lit(v1.x + v2.x, v1.y + v2.y); }
static Vec2 Vec2Sub(Vec2 v1, Vec2 v2) { return Vec2Lit(v1.x - v2.x, v1.y - v2.y); }
static Vec2 Vec2Scale(Vec2 v, f32 s) { return Vec2Lit(v.x * s, v.y * s); }
static Vec2 Vec2Clamp(Vec2 v1, Vec2 min, Vec2 max) {
    return Vec2Lit(f32max(min.x, f32min(v1.x, max.x)), f32max(min.y, f32min(v1.y, max.y)));
}
static f32 Vec2Len(Vec2 v) { return sqrtf32((v.x * v.x) + (v.y * v.y)); }
static f32 Vec2Dot(Vec2 v1, Vec2 v2) { return (v1.x * v2.x) + (v1.y * v2.y); }

typedef struct Vec3 {
    f32 x;
    f32 y;
    f32 z;
} Vec3;

#define Vec3Lit(a, b, c) ((Vec3){.x = (a), .y = (b), .z = (c)})
#define Vec3ToVec2(v) (Vec2Lit((v).x, (v).y))
#define Vec2ToVec3(v) (Vec3Lit((v).x, (v).y, 0.0f))
#define Vec3ToRVector3(v) ((Vector3){.x = (v).x, .y = (v).y, .z = (v.z)})

static Vec3 Vec3Zero() {
    static Vec3 v = Vec3Lit(0.0f, 0.0f, 0.0f);
    return v;
}

static Vec3 Vec3Add(Vec3 v1, Vec3 v2) { return Vec3Lit(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z); }
static Vec3 Vec3Sub(Vec3 v1, Vec3 v2) { return Vec3Lit(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z); }
static Vec3 Vec3Scale(Vec3 v, f32 n) { return Vec3Lit(v.x * n, v.y * n, v.z * n); }
static f32 Vec3Len(Vec3 v) { return sqrtf32((v.x * v.x) + (v.y * v.y) + (v.z * v.z)); }
static f32 Vec3Dot(Vec3 v1, Vec3 v2) { return (v1.x * v2.x) + (v1.y * v2.y) + (v1.z * v2.z); }

////////////////////////////////////////////////////////////
// S:Collision

typedef struct CollisionInfo {
} CollisionInfo;

////////////////////////////////////////////////////////////
// S:Physics

typedef struct Body {
    bool active;
    Vec2 pos;
    Vec2 vel;

    f32 radius;
} Body;

static void BodyInit(Body* body) { memset(body, 0, sizeof(*body)); }

typedef struct Collider {
    bool collision;

    // TODO: Pull into contact point/collision point struct later.
    // Unit vector in the direction of the collision normal for body1.
    Vec2 normal;
    f32 depth;
} Collider;

static void ColliderInit(Collider* collider, Body* body1, Body* body2) {
    memset(collider, 0, sizeof(*collider));
    collider->collision = true;

    // SAT
    {
        // Circle
        {
            Vec2 relative_distance_dir = Vec2Sub(body1->pos, body2->pos);
            f32 relative_distance = Vec2Len(relative_distance_dir);
            f32 relative_distance_constraint = (body1->radius + body2->radius);
            f32 depth = relative_distance - relative_distance_constraint;
            if (depth >= 0.0f) {
                collider->collision = false;
                return;
            }

            collider->depth = f32abs(depth);
            // Normalize relative position. This is the collision normal (there is only a single potential
            // separating axis for circles).
            collider->normal =
                Vec2Lit(relative_distance_dir.x / relative_distance, relative_distance_dir.y / relative_distance);
        }

        // // AABB Rect.
        // {
        //     f32 body1_half_width = fhalf(body1->width);
        //     f32 min_x_body1 = body1->pos.x - body1_half_width;
        //     f32 max_x_body1 = body1->pos.x + body1_half_width;

        //     f32 body2_half_width = fhalf(body2->width);
        //     f32 min_x_body2 = body2->pos.x - body2_half_width;
        //     f32 max_x_body2 = body2->pos.x + body2_half_width;
        //     collision &= max_x_body1 >= min_x_body2 && max_x_body2 >= min_x_body1;

        //     f32 body1_half_height = fhalf(body1->height);
        //     f32 min_y_body1 = body1->pos.y - body1_half_height;
        //     f32 max_y_body1 = body1->pos.y + body1_half_height;

        //     f32 body2_half_height = fhalf(body2->height);
        //     f32 min_y_body2 = body2->pos.y - body2_half_height;
        //     f32 max_y_body2 = body2->pos.y + body2_half_height;
        //     collision &= max_y_body1 >= min_y_body2 && max_y_body2 >= min_y_body1;
        // }
    }
}

typedef u32 ParticleId;

typedef struct Constraint {
    bool active;
    ParticleId pid_a;
    ParticleId pid_b;
    f32 rest_len;
} Constraint;

#define MAX_PARTICLES 5
#define MAX_CONSTRAINTS 5
#define MAX_ITERATIONS 10

typedef struct PhysicsState {
    f32 fixed_dt;
    Vec2 gravity;

    bool active[MAX_PARTICLES];
    Vec2 pos[MAX_PARTICLES];
    Vec2 pos_old[MAX_PARTICLES];
    Vec2 force[MAX_PARTICLES];
    Constraint constraints[MAX_CONSTRAINTS];

} PhysicsState;

#define ParticleGetPos(pstate, id) ((pstate)->pos[(id)])

static void PhysicsStateInit(PhysicsState* pstate) {
    memset(pstate, 0, sizeof(*pstate));
    pstate->fixed_dt = 1.0f / 60.0f;
}

static ParticleId PhysicsStateAddParticle(PhysicsState* pstate, Vec2 pos) {
    for (ParticleId i = 0; i < MAX_PARTICLES; ++i) {
        if (!pstate->active[i]) {
            pstate->active[i] = true;
            pstate->pos[i] = pos;
            pstate->pos_old[i] = pos;
            return i;
        }
    }

    Assert(false);
    return 0;
}

static Constraint* PhysicsStateAllocateConstraint(PhysicsState* pstate) {
    for (u32 i = 0; i < MAX_CONSTRAINTS; ++i) {
        if (!pstate->constraints[i].active) {
            Constraint c = {0};
            Constraint* cc = &pstate->constraints[i];
            *cc = c;
            cc->active = true;
            return cc;
        }
    }

    Assert(false);
    return NULL;
}

static void PhysicsStateAccumulateForces(PhysicsState* pstate) {
    for (ParticleId i = 0; i < MAX_PARTICLES; ++i) {
        pstate->force[i] = Vec2Scale(pstate->gravity, pstate->active[i]);
    }
}
static void PhysicsStateIntegrate(PhysicsState* pstate) {
    f32 dt_squared = pstate->fixed_dt * pstate->fixed_dt;
    for (ParticleId i = 0; i < MAX_PARTICLES; ++i) {
        // x_new = 2x - x_old + force * dt^2
        Vec2 cur_x = pstate->pos[i];
        Vec2 dx = Vec2Add(Vec2Sub(cur_x, pstate->pos_old[i]), Vec2Scale(pstate->force[i], dt_squared));
        pstate->pos[i] = Vec2Add(cur_x, Vec2Scale(dx, pstate->active[i]));
        pstate->pos_old[i] = cur_x;
    }
}
static void PhysicsStateSatisfyConstraints(PhysicsState* pstate) {
    f32 d = 100.0f;
    Vec2 minv = Vec2Lit(-d, -d);
    Vec2 maxv = Vec2Lit(d, d);
    for (u32 j = 0; j < MAX_ITERATIONS; ++j) {
        for (ParticleId i = 0; i < MAX_PARTICLES; ++i) {
            pstate->pos[i] = Vec2Clamp(pstate->pos[i], minv, maxv);
        }

        for (u32 k = 0; k < MAX_CONSTRAINTS; ++k) {
            Constraint c = pstate->constraints[k];
            if (!c.active) {
                continue;
            }

            Vec2 x0 = pstate->pos[c.pid_a];
            Vec2 x1 = pstate->pos[c.pid_b];
            Vec2 delta = Vec2Sub(x0, x1);
            f32 delta_len = Vec2Len(delta);
            f32 diff = (delta_len - c.rest_len) / delta_len;
            pstate->pos[c.pid_a] = Vec2Sub(x0, Vec2Scale(delta, 0.5f * diff));
            pstate->pos[c.pid_b] = Vec2Add(x1, Vec2Scale(delta, 0.5f * diff));
        }
    }
}

static void PhysicsStateTick(PhysicsState* pstate) {
    PhysicsStateAccumulateForces(pstate);
    PhysicsStateIntegrate(pstate);
    PhysicsStateSatisfyConstraints(pstate);
}

////////////////////////////////////////////////////////////

const float RenderWidth = 1280;
const float RenderHeight = 720;

static const Vector3 FLIP_Y = {1, -1, 1};

static const Vec3 ORIGIN = Vec3Lit(0.0f, 0.0f, 0.0f);
static const Vector2 ORIGIN2D = {ORIGIN.x, ORIGIN.y};

////////////////////////////////////////////////////////////

#define ANIMATION_FRAME_CAP (4)

////////////////////////////////////////////////////////////
typedef enum EntityFlag {
    EFLAG_NONE = 0,
    EFLAG_ACTIVE = 1 << 0,
    EFLAG_DRAW = 1 << 1,
    EFLAG_PLAYER_CONTROLLED = 1 << 2,
    EFLAG_PHYSICS_ON = 1 << 3
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

    Body* body;
    ParticleId particle_id;

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

    PhysicsState pstate;

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
#define GetPhysicsState(gstate) (&(gstate)->pstate)

static GameState GState;

#define GetGameState() (&GState)

static void SimpleScene(GameState* state) {
    // LoadLevel();

    state->score = 0.0f;
    state->health = 3;

    // Entity 0.
    {
        Entity* entity = AllocateEntity();
        FlagOn(entity->flags, EFLAG_PLAYER_CONTROLLED);
        int width = 24 * 3;
        int height = 24 * 3;
        entity->p = ORIGIN;

        f32 radius = fhalf(width);
        Vec2 center = Vec2Lit(radius, radius);
        Image image = GenImageColor(width, height, BLANK);
        ImageDrawCircleV(&image, Vec2ToRVector2(center), radius, GRAY);
        entity->texture = LoadTextureFromImage(image);
        entity->bitmap_rec = (Rectangle){0, 0, width, height};

        FlagOn(entity->flags, EFLAG_PHYSICS_ON);
        entity->particle_id = PhysicsStateAddParticle(&state->pstate, Vec3ToVec2(entity->p));
    }

    // Entity 1.
    {
        Entity* entity = AllocateEntity();
        FlagOn(entity->flags, EFLAG_PLAYER_CONTROLLED);
        int width = 24 * 3;
        int height = 24 * 3;
        entity->p = Vec3Lit(10.0f, -10.0f, 0.0f);

        f32 radius = fhalf(width);
        Vec2 center = Vec2Lit(radius, radius);
        Image image = GenImageColor(width, height, BLANK);
        ImageDrawCircleV(&image, Vec2ToRVector2(center), radius, GRAY);
        entity->texture = LoadTextureFromImage(image);
        entity->bitmap_rec = (Rectangle){0, 0, width, height};

        FlagOn(entity->flags, EFLAG_PHYSICS_ON);
        entity->particle_id = PhysicsStateAddParticle(&state->pstate, Vec3ToVec2(entity->p));
    }

    {
        Constraint* c = PhysicsStateAllocateConstraint(GetPhysicsState(state));
        c->pid_a = 0;
        c->pid_b = 1;
        c->rest_len = 40.0f;
    }

    // f32 disp = 100.0f;
    // u32 i = 0;
    // Color colors[4] = {RED, BLUE, GREEN, PURPLE};
    // for (i32 dir = -1; dir < 2; dir += 2) {
    //     {
    //         Color color = colors[i++];

    //         Entity* entity = AllocateEntity();
    //         FlagOn(entity->flags, EFLAG_PLAYER_CONTROLLED);
    //         int width = 24 * 3;
    //         int height = 24 * 3;
    //         entity->p = Vec3Lit(disp * dir, 0.0f, 0.0f);

    //         f32 radius = fhalf(width);
    //         Vec2 center = Vec2Lit(radius, radius);
    //         Image image = GenImageColor(width, height, BLANK);
    //         ImageDrawCircleV(&image, Vec2ToRVector2(center), radius, color);
    //         entity->texture = LoadTextureFromImage(image);
    //         entity->bitmap_rec = (Rectangle){0, 0, width, height};

    //         FlagOn(entity->flags, EFLAG_PHYSICS_ON);
    //         entity->body = PhysicsStateAllocateBody(&state->pstate);
    //         entity->body->pos = Vec3ToVec2(entity->p);
    //         entity->body->vel = Vec2Lit(disp * (-1.0f * dir), 0.0f);
    //         entity->body->radius = fhalf(width);
    //     }

    //     {
    //         Color color = colors[i++];

    //         Entity* entity = AllocateEntity();
    //         FlagOn(entity->flags, EFLAG_PLAYER_CONTROLLED);
    //         int width = 24 * 3;
    //         int height = 24 * 3;
    //         entity->p = Vec3Lit(0.0f, disp * dir, 0.0f);

    //         f32 radius = fhalf(width);
    //         Vec2 center = Vec2Lit(radius, radius);
    //         Image image = GenImageColor(width, height, BLANK);
    //         ImageDrawCircleV(&image, Vec2ToRVector2(center), radius, color);
    //         entity->texture = LoadTextureFromImage(image);
    //         entity->bitmap_rec = (Rectangle){0, 0, width, height};

    //         FlagOn(entity->flags, EFLAG_PHYSICS_ON);
    //         entity->body = PhysicsStateAllocateBody(&state->pstate);
    //         entity->body->pos = Vec3ToVec2(entity->p);
    //         entity->body->vel = Vec2Lit(0.0f, disp * (-1.0f * dir));
    //         entity->body->radius = fhalf(width);
    //     }
    // }
}

static void GameStateInit(GameState* state) {
    memset(state, 0, sizeof(GameState));

    PhysicsState* pstate = &state->pstate;
    PhysicsStateInit(pstate);
    pstate->gravity = Vec2Lit(0.0f, -10.0f);
    // pstate->gravity = Vec2Zero();

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

    // PhysicsStateUpdate(&state->pstate, dt);
    if (!IsKeyDown(KEY_SPACE)) {
        PhysicsStateTick(&state->pstate);
    }

    EntityManager* em = GetEntityManager();
    for (uint32_t i = 0; i < ArraySizeOf(em->entities); ++i) {
        Entity* entity = GetEntity(i);
        if (!TestFlag(entity->flags, EFLAG_ACTIVE)) {
            continue;
        }

        if (TestFlag(entity->flags, EFLAG_PHYSICS_ON)) {
            entity->p = Vec2ToVec3(ParticleGetPos(&state->pstate, entity->particle_id));
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
        Vector3 p = Vec3ToRVector3(entity->p);
        // DrawTextureRec(entity->texture, src, (Vector2){p.x - half_width, p.y - half_height}, tint);
        if (true) {
            DrawCircle(p.x, p.y, 3, RED);
        }
    }

    DrawRectangleLines(-100.0f, -100.0f, 200.0f, 200.0f, BLACK);

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