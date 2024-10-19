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

#define F32_MAX FLT_MAX

////////////////////////////////////////////////////////////
// S:Math

#define f32abs fabs
#define f32min fminf
#define f32max fmaxf
#define sqrtf32 sqrtf
#define fhalf(x) ((x) * 0.5f)
#define f32sin sin

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
static Vec2 Vec2Normalize(Vec2 v) {
    Vec2 result = Vec2Lit(v.x, v.y);
    f32 len = sqrtf32((v.x * v.x) + (v.y * v.y));
    if (len > 0.0f) {
        f32 inv_len = 1.0f / len;
        result.x *= inv_len;
        result.y *= inv_len;
    }
    return result;
}
static Vec2 Vec2Clamp(Vec2 v1, Vec2 min, Vec2 max) {
    return Vec2Lit(f32max(min.x, f32min(v1.x, max.x)), f32max(min.y, f32min(v1.y, max.y)));
}
static Vec2 Vec2Cross(Vec2 v) { return Vec2Lit(v.y, -v.x); }
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
static Vec3 Vec3Normalize(Vec3 v) {
    Vec3 result = Vec3Lit(v.x, v.y, v.z);
    f32 len = sqrtf32((v.x * v.x) + (v.y * v.y) + (v.z * v.z));
    if (len > 0.0f) {
        f32 inv_len = 1.0f / len;
        v.x *= inv_len;
        v.y *= inv_len;
        v.z *= inv_len;
    }
    return result;
}
static f32 Vec3Len(Vec3 v) { return sqrtf32((v.x * v.x) + (v.y * v.y) + (v.z * v.z)); }
static f32 Vec3Dot(Vec3 v1, Vec3 v2) { return (v1.x * v2.x) + (v1.y * v2.y) + (v1.z * v2.z); }

////////////////////////////////////////////////////////////
// S:Collision

// CCW winding.
// If collision, normal will point in direction of separating axis towards triangle ABC.
bool CollideTriangleTriangle(Vec2 a, Vec2 b, Vec2 c, Vec2 d, Vec2 e, Vec2 f, Vec2* normal, f32* depth) {
    Vec2 edges[6] = {Vec2Sub(b, a), Vec2Sub(c, b), Vec2Sub(a, c), Vec2Sub(e, d), Vec2Sub(f, e), Vec2Sub(d, f)};

    *depth = F32_MAX;

    // SAT.
    for (u8 i = 0; i < 6; ++i) {
        Vec2 edge = edges[i];
        Vec2 n = Vec2Normalize(Vec2Cross(edge));
        f32 proj_a = Vec2Dot(a, n);
        f32 proj_b = Vec2Dot(b, n);
        f32 proj_c = Vec2Dot(c, n);

        f32 proj_d = Vec2Dot(d, n);
        f32 proj_e = Vec2Dot(e, n);
        f32 proj_f = Vec2Dot(f, n);

        f32 max_abc = f32max(proj_a, f32max(proj_b, proj_c));
        f32 min_def = f32min(proj_d, f32min(proj_e, proj_f));

        if (max_abc < min_def) {
            return false;
        }

        f32 min_abc = f32min(proj_a, f32min(proj_b, proj_c));
        f32 max_def = f32max(proj_d, f32max(proj_e, proj_f));

        if (min_abc > max_def) {
            return false;
        }

        f32 d = f32abs((max_abc - min_abc) - (max_def - min_def));
        if (d < *depth) {
            *depth = d;
            *normal = n;
        }
    }

    return true;
}

////////////////////////////////////////////////////////////
// S:Physics

typedef u32 ParticleId;

typedef u32 ConstraintId;

typedef struct Constraint {
    bool active;
    ParticleId pid_a;
    ParticleId pid_b;
    f32 rest_len;
} Constraint;

typedef u32 BodyId;

typedef struct Body {
    bool active;

    // Only support triangles, CCW order.
    ParticleId pids[3];
} Body;

static void BodyInit(Body* body) {
    static Body ZERO = {0};
    *body = ZERO;
}

#define MAX_PARTICLES 10
#define MAX_CONSTRAINTS 10
#define MAX_ITERATIONS 10
#define MAX_BODIES 10

typedef struct PhysicsState {
    f32 fixed_dt;
    Vec2 gravity;

    bool active[MAX_PARTICLES];
    Vec2 pos[MAX_PARTICLES];
    Vec2 pos_old[MAX_PARTICLES];
    Vec2 force[MAX_PARTICLES];
    Constraint constraints[MAX_CONSTRAINTS];
    Body body[MAX_BODIES];

} PhysicsState;

#define ParticleGetPos(pstate, id) ((pstate)->pos[(id)])
#define GetBody(pstate, bid) (&(pstate)->body[(bid)])
#define GetConstraint(pstate, cid) (&(pstate)->constraints[(cid)])

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
            // Cube constraint.
            pstate->pos[i] = Vec2Clamp(pstate->pos[i], minv, maxv);
        }

        for (u32 k = 0; k < MAX_CONSTRAINTS; ++k) {
            Constraint c = pstate->constraints[k];
            if (!c.active) {
                continue;
            }

            // Stick constraint.
            Vec2 x0 = pstate->pos[c.pid_a];
            Vec2 x1 = pstate->pos[c.pid_b];
            Vec2 delta = Vec2Sub(x0, x1);
            f32 delta_len = Vec2Len(delta);
            f32 diff = (delta_len - c.rest_len) / delta_len;
            pstate->pos[c.pid_a] = Vec2Sub(x0, Vec2Scale(delta, 0.5f * diff));
            pstate->pos[c.pid_b] = Vec2Add(x1, Vec2Scale(delta, 0.5f * diff));
        }

        Vec2 normal = Vec2Zero();
        f32 depth = 0.0f;
        if (CollideTriangleTriangle(pstate->pos[0], pstate->pos[1], pstate->pos[2], pstate->pos[3], pstate->pos[4],
                                    pstate->pos[5], &normal, &depth)) {
            // pstate->active[0] = false;
            // pstate->active[1] = false;
            // pstate->active[2] = false;
            // pstate->active[3] = false;
            // pstate->active[4] = false;
            // pstate->active[5] = false;

            Vec2 disp = Vec2Scale(normal, 0.5f * depth);
            Vec2 neg_disp = Vec2Scale(disp, -1.0f);
            pstate->pos[0] = Vec2Add(pstate->pos[0], disp);
            pstate->pos[1] = Vec2Add(pstate->pos[1], disp);
            pstate->pos[2] = Vec2Add(pstate->pos[2], disp);

            pstate->pos[3] = Vec2Add(pstate->pos[3], neg_disp);
            pstate->pos[4] = Vec2Add(pstate->pos[4], neg_disp);
            pstate->pos[5] = Vec2Add(pstate->pos[5], neg_disp);

            // breakpoint();
        }
    }
}

static void PhysicsStateTick(PhysicsState* pstate) {
    PhysicsStateAccumulateForces(pstate);
    PhysicsStateIntegrate(pstate);
    PhysicsStateSatisfyConstraints(pstate);
}

static void PhysicsStateDraw(PhysicsState* pstate) {
    for (ConstraintId cid = 0; cid < MAX_CONSTRAINTS; ++cid) {
        Constraint* c = GetConstraint(pstate, cid);
        if (!c->active) {
            continue;
        }
        Vec2 pa = ParticleGetPos(pstate, c->pid_a);
        Vec2 pb = ParticleGetPos(pstate, c->pid_b);
        DrawLineEx(Vec2ToRVector2(pa), Vec2ToRVector2(pb), 2, BLACK);
    }

    for (ParticleId i = 0; i < MAX_PARTICLES; ++i) {
        if (!pstate->active[i]) {
            continue;
        }

        Vec2 p = ParticleGetPos(pstate, i);
        DrawCircle(p.x, p.y, 3, RED);
    }
}

////////////////////////////////////////////////////////////

const float RenderWidth = 1280;
const float RenderHeight = 720;

static const Vector3 FLIP_Y = {1, -1, 1};

static const Vec3 ORIGIN = Vec3Lit(0.0f, 0.0f, 0.0f);
static const Vec2 ORIGIN2D = {ORIGIN.x, ORIGIN.y};

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

typedef u32 EntityId;

typedef struct Entity {
    /////////////////////////////////////////////////////////////////
    // Core field.
    EntityId id;
    Vec3 pos;
    EntityFlag flags;

    /////////////////////////////////////////////////////////////////
    // Gameplay fields.

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
    Camera camera;

    bool debug;
    bool game_over;

    float timer;

    RenderTexture2D target;

    PhysicsState pstate;

    /////////////////////////////////////////
    // Gameplay fields

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

    // Entity 0.
    {
        Entity* entity = AllocateEntity();
        FlagOn(entity->flags, EFLAG_PLAYER_CONTROLLED);
        entity->pos = ORIGIN;
    }
}

static void GameStateInit(GameState* gstate) {
    memset(gstate, 0, sizeof(GameState));

    PhysicsState* pstate = &gstate->pstate;
    PhysicsStateInit(pstate);
    pstate->gravity = Vec2Lit(0.0f, -10.0f);
    // pstate->gravity = Vec2Zero();

    InitWindow(RenderWidth, RenderHeight, "Game");
    SetWindowState(FLAG_WINDOW_UNDECORATED);
    // ToggleFullscreen();
    SetConfigFlags(FLAG_VSYNC_HINT);
    SetTargetFPS(60);
    EntitySystemInit();

    gstate->target = LoadRenderTexture(RenderWidth, RenderHeight);

    SimpleScene(gstate);

    {
        Camera camera = {0};
        camera.position = Vec3ToRVector3(Vec3Lit(0.0f, 20.0f, 40.0f));
        camera.target = Vec3ToRVector3(ORIGIN);
        camera.up = Vec3ToRVector3(Vec3Lit(0.0f, 1.0f, 0.0f));
        camera.fovy = 60.0f;
        camera.projection = CAMERA_PERSPECTIVE;
        gstate->camera = camera;
    }
}

static void GameStateUpdate(GameState* gstate, float dt) {
    gstate->timer += dt;

    // PhysicsStateUpdate(&state->pstate, dt);
    if (!IsKeyDown(KEY_SPACE)) {
        PhysicsStateTick(&gstate->pstate);
    }

    EntityManager* em = GetEntityManager();
    for (uint32_t i = 0; i < ArraySizeOf(em->entities); ++i) {
        Entity* entity = GetEntity(i);
        if (!TestFlag(entity->flags, EFLAG_ACTIVE)) {
            continue;
        }

        if (TestFlag(entity->flags, EFLAG_PLAYER_CONTROLLED)) {
            static const u8 GAMEPAD_ID = 0;

            f32 x = GetGamepadAxisMovement(GAMEPAD_ID, GAMEPAD_AXIS_LEFT_X);
            f32 y = GetGamepadAxisMovement(GAMEPAD_ID, GAMEPAD_AXIS_LEFT_Y);

            // TODO: auto align movement direction to camera position.
            Vec3 dir = Vec3Normalize(Vec3Lit(x, 0.0f, y));
            entity->pos = Vec3Add(entity->pos, Vec3Scale(dir, 30.0f * dt));
        }
    }

    gstate->debug ^= (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_ONE));
}

typedef enum AttLoc {
    LOC_POS_X = 0,
    LOC_POS_Y,
    LOC_POS_Z,

    LOC_COLOR_R,
    LOC_COLOR_G,
    LOC_COLOR_B,

    LOC_COUNT
} AttLoc;

typedef struct VAtt {
    f32 atts[LOC_COUNT];
} Vatt;

static void GameStateDraw(GameState* gstate, float delta) {
    i32 success = 0;

    const char* vertex_shader_source =
        "#version 330 core\n"
        "layout (location = 0) in vec3 InPos;\n"
        "layout (location = 1) in vec3 InColor;\n"
        "\n"
        "out vec4 VertColor;\n"
        "void main()\n"
        "{\n"
        "   gl_Position = vec4(InPos.x, InPos.y, InPos.z, 1.0);\n"
        "   VertColor = vec4(InColor.x, InColor.y, InColor.z, 1.0);\n"
        "}\0";
    u32 vshader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vshader, 1, &vertex_shader_source, NULL);
    glCompileShader(vshader);
    glGetShaderiv(vshader, GL_COMPILE_STATUS, &success);
    Assert(success);

    const char* fshader_source =
        "#version 330 core\n"
        "in vec4 VertColor;\n"
        "out vec4 FragColor;\n"
        "uniform vec4 UColor;\n"
        "void main()\n"
        "{\n"
        "    FragColor = (0.5 * UColor) + (0.5 * VertColor);\n"
        "}\0";

    u32 fshader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fshader, 1, &fshader_source, NULL);
    glCompileShader(fshader);
    glGetShaderiv(fshader, GL_COMPILE_STATUS, &success);
    Assert(success);

    u32 shader_program = glCreateProgram();
    glAttachShader(shader_program, vshader);
    glAttachShader(shader_program, fshader);
    glLinkProgram(shader_program);
    glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
    Assert(success);

    i32 vpos_loc = glGetAttribLocation(shader_program, "InPos");
    i32 vcolor_loc = glGetAttribLocation(shader_program, "InColor");

    f32 greenValue = (f32sin(gstate->timer) / 2.0f) + 0.5f;
    i32 color_loc = glGetUniformLocation(shader_program, "UColor");

    u32 vao1 = 0;
    glGenVertexArrays(1, &vao1);
    glBindVertexArray(vao1);

    f32 vertices1[] = {
        //
        -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 0.0f,
        //
        0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 0.0f,
        //
        0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f
        //
    };

    u32 vb1 = 0;
    glGenBuffers(1, &vb1);
    glBindBuffer(GL_ARRAY_BUFFER, vb1);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices1), vertices1, GL_STATIC_DRAW);

    glVertexAttribPointer(vpos_loc, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(vpos_loc);
    glVertexAttribPointer(vcolor_loc, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(vcolor_loc);

    u32 vao2 = 0;
    glGenVertexArrays(1, &vao2);
    glBindVertexArray(vao2);

    f32 vertices2[] = {
        //
        -0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f,
        //
        0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,
        //
        0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f
        //
    };

    u32 vb2 = 0;
    glGenBuffers(1, &vb2);
    glBindBuffer(GL_ARRAY_BUFFER, vb2);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices2), vertices2, GL_STATIC_DRAW);

    glVertexAttribPointer(vpos_loc, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(vpos_loc);
    glVertexAttribPointer(vcolor_loc, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(vcolor_loc);

    BeginDrawing();

    ClearBackground(WHITE);

    BeginMode3D(gstate->camera);

    glUseProgram(shader_program);
    glUniform4f(color_loc, 0.0f, greenValue, 0.0f, 1.0f);

    glBindVertexArray(vao1);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(vao2);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    // EntityManager* em = GetEntityManager();
    // for (uint32_t i = 0; i < ArraySizeOf(em->entities); ++i) {
    //     Entity* entity = GetEntity(i);
    //     if (!TestFlag(entity->flags, EFLAG_ACTIVE)) {
    //         continue;
    //     }

    //     u16 len = 5;
    //     DrawSphereWires(Vec3ToRVector3(entity->pos), len, 10, 15, RED);
    // }

    // DrawPlane(Vec3ToRVector3(ORIGIN), Vec2ToRVector2(Vec2Lit(100, 100)), GRAY);

    // {
    //     static bool init = false;
    //     static Mesh mesh = {0};
    //     if (!init) {
    //         init = true;
    //         f32 len = 5.0f;
    //         mesh = GenMeshCube(len, len, 100.0f);
    //     }
    //     Model model = LoadModelFromMesh(mesh);
    //     model.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = BLACK;
    //     DrawModelEx(model, Vec3ToRVector3(Vec3Lit(-20.0f, 0.0f, 0.0f)), (Vector3){0.0f, 0.0f, -1.0f}, 15.0f,
    //                 Vector3One(), WHITE);
    // }

    EndMode3D();

    if (gstate->debug) {
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