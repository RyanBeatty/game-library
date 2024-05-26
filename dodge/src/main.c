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

    Texture texture;
    // Rectangle source;
} Entity;

typedef struct EntityEntry {
    EntityId key;
    Entity value;
} EntityEntry;

typedef struct EntityManager {
    EntityEntry* entities;
} EntityManager;

static EntityManager EntityM = {0};

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

////////////////////////////////////////////////////////////

typedef struct GameState {
    Camera camera;

    EntityId* entity_ids;
} GameState;

static GameState GState;

static void SimpleScene(GameState* state) {
    {
        Entity* entity = MakeEntity(1);
        float y = GetScreenHeight() * 0.65;
        entity->p = (Vector3){0, y, 0};
        entity->texture = LoadTextureFromImage(GenImageColor(GetScreenWidth(), y * 2.0f, GRAY));
        arrpush(state->entity_ids, entity->id);
    }
}

static void GameStateInit(GameState* state) {
    memset(state, 0, sizeof(GameState));
    InitWindow(ScreenWidth, ScreenHeight, "Game");
    SetTargetFPS(60);

    SimpleScene(state);
}

static void GameStateDraw(GameState* state) {
    BeginDrawing();

    ClearBackground(RAYWHITE);

    for (uint32_t i = 0; i < arrlenu(state->entity_ids); ++i) {
        Entity* entity = GetEntity(state->entity_ids[i]);
        DrawTexture(entity->texture, entity->p.x, entity->p.y, WHITE);
    }

    EndDrawing();
}

static void GameStateUpdate(GameState* state, float delta_time) {}

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