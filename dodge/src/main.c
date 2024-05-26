#ifndef UNITY_BUILD
#include <stdlib.h>
#include <time.h>

#include "raylib.h"
#include "raymath.h"

#endif

const int ScreenWidth = 800;
const int ScreenHeight = 450;

////////////////////////////////////////////////////////////
static size_t GenerateRandomSeed() {
    // Get current time in seconds since Epoch
    time_t currentTime = time(NULL);

    // Generate a random seed using current time and clock ticks
    size_t seed = (size_t)(currentTime * 1000) + (size_t)clock();

    return seed;
}

static size_t GetSeed() {
    static size_t Seed = 0;
    while (Seed == 0) {
        Seed = GenerateRandomSeed();
    }
    return Seed;
}

////////////////////////////////////////////////////////////

// typedef size_t TextureId;

// #define TextureIdFromU32(str) stbds_hash_string(str, GetSeed())

// typedef struct TextureEntry {
//     TextureId key;
//     Texture value;
// }

// typedef struct TextureCache {
//     TextureEntry* textures;
// } TextureCache;

// static TextureCache TextureC;

// static Texture GetTexture(const char* name) {
//     TextureId id = TextureIdFromU32(name);
//     ptrdiff_t idx = hmgeti(EntityC.textures, id);
//     bool first_frame = idx == -1;
//     if (first_frame) {
//         Texture texture = LoadTexture(name);
//         hmput(TextureC.textures, id, texture);
//         idx = hmgeti(TextureC.texture, id);
//         Assert(idx != -1);
//     }
//     Entity* entity = &TextureC.textures[idx].value;
//     return entity;
// }

////////////////////////////////////////////////////////////
typedef enum EntityFlag { EFLAG_NONE = 0, EFALG_PLAYER_CONTROLLED = 1 << 0 } EntityFlag;

typedef uint32_t EntityId;

static EntityId EntityIdFromU32(uint32_t id) {
    size_t key = stbds_hash_bytes(&id, sizeof(id), GetSeed());
    Assert(key != 0);
    return key;
}

typedef struct Entity {
    EntityId id;
    Vector3 p;
    Quaternion rot;
    EntityFlag flags;

    Texture texture;
    Rectangle rec;
} Entity;

typedef struct EntityEntry {
    EntityId key;
    Entity value;
} EntityEntry;

typedef struct EntityManager {
    EntityEntry* entities;
} EntityManager;

static EntityManager EntityM = {0};

static bool EntityTestFlags(Entity* entity, EntityFlag flags) { return (entity->flags & flags) == flags; }

static void EntitySystemInit() { memset(&EntityM, 0, sizeof(EntityM)); }

static Entity* MakeEntity(EntityId id) {
    ptrdiff_t idx = hmgeti(EntityM.entities, id);
    bool first_frame = idx == -1;
    if (first_frame) {
        Entity entity = {0};
        entity.id = id;
        hmput(EntityM.entities, id, entity);
        idx = hmgeti(EntityM.entities, id);
        Assert(idx != -1);
    }
    Entity* entity = &EntityM.entities[idx].value;
    return entity;
}

static Entity* GetEntity(EntityId id) {
    EntityEntry* entry = hmgetp_null(EntityM.entities, id);
    if (entry == NULL) {
        return NULL;
    }
    return &entry->value;
}

static void EntityUpdate(Entity* entity, float delta) {
    if (EntityTestFlags(entity, EFALG_PLAYER_CONTROLLED)) {
        Vector3 direction = Vector3Zero();
        if (IsKeyDown(KEY_RIGHT)) {
            direction = Vector3Add(direction, (Vector3){1, 0, 0});
        }
        if (IsKeyDown(KEY_LEFT)) {
            direction = Vector3Add(direction, (Vector3){-1, 0, 0});
        }

        float speed = 2.0f;
        entity->p = Vector3Add(entity->p, Vector3Scale(direction, speed));
    }
}

static void EntityDraw(Entity* entity) {
    DrawTextureRec(entity->texture, entity->rec, (Vector2){entity->p.x, entity->p.y}, WHITE);
}

////////////////////////////////////////////////////////////

typedef struct GameState {
    Camera camera;

    EntityId* entity_ids;
} GameState;

static GameState GState;

static void SimpleScene(GameState* state) {
    {
        Entity* entity = MakeEntity(EntityIdFromU32(1));
        float y = GetScreenHeight() * 0.65;
        entity->p = (Vector3){0, y, 0};
        int width = GetScreenWidth();
        int height = y * 2.0f;
        entity->texture = LoadTextureFromImage(GenImageColor(width, height, GRAY));
        entity->rec = (Rectangle){0, 0, width, height};
        arrpush(state->entity_ids, entity->id);
    }

    {
        Entity* ground = GetEntity(EntityIdFromU32(1));
        Assert(ground != NULL);
        Entity* entity = MakeEntity(EntityIdFromU32(2));
        entity->flags = EFALG_PLAYER_CONTROLLED;
        int width = 28;
        int height = 40;
        float y = ground->p.y - height;
        entity->p = (Vector3){GetScreenWidth() * 0.5, y, 0};
        entity->texture = LoadTextureFromImage(GenImageColor(width, height, BLUE));
        entity->rec = (Rectangle){0, 0, width, height};
        arrpush(state->entity_ids, entity->id);
    }
}

static void GameStateInit(GameState* state) {
    memset(state, 0, sizeof(GameState));
    InitWindow(ScreenWidth, ScreenHeight, "Game");
    SetTargetFPS(60);
    EntitySystemInit();

    SimpleScene(state);
}

static void GameStateDraw(GameState* state) {
    BeginDrawing();

    ClearBackground(RAYWHITE);

    for (uint32_t i = 0; i < arrlenu(state->entity_ids); ++i) {
        Entity* entity = GetEntity(state->entity_ids[i]);
        EntityDraw(entity);
    }

    EndDrawing();
}

static void GameStateUpdate(GameState* state, float delta) {
    for (uint32_t i = 0; i < arrlenu(state->entity_ids); ++i) {
        Entity* entity = GetEntity(state->entity_ids[i]);
        EntityUpdate(entity, delta);
    }
}

////////////////////////////////////////////////////////////

int main(void) {
    InitWindow(ScreenWidth, ScreenHeight, "Game");
    GameStateInit(&GState);

    while (!WindowShouldClose()) {
        float delta_time = GetFrameTime();
        GameStateUpdate(&GState, delta_time);
        GameStateDraw(&GState);
    }

    CloseWindow();

    return 0;
}