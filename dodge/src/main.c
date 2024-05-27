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
typedef enum EntityFlag { EFLAG_NONE = 0, EFLAG_PLAYER_CONTROLLED = 1 << 0, EFLAG_MOVE_RANDOM = 2 << 0 } EntityFlag;

typedef uint32_t EntityId;

static EntityId EntityIdFromU32(uint32_t id) {
    size_t key = stbds_hash_bytes(&id, sizeof(id), GetSeed());
    Assert(key != 0);
    return key;
}

static EntityId NextEntityId() {
    static uint32_t id = 0;
    EntityId eid = EntityIdFromU32(id);
    ++id;
    return eid;
}

typedef struct Entity {
    EntityId id;
    Vector3 p;
    Quaternion rot;
    EntityFlag flags;

    Texture texture;
    Rectangle bitmap_rec;
} Entity;

typedef struct EntityEntry {
    EntityId key;
    Entity value;
} EntityEntry;

typedef struct EntityManager {
    EntityEntry* entities;
    EntityId* entity_ids;
} EntityManager;

static EntityManager EntityM = {0};

#define GetEntityManager() &EntityM

static bool EntityTestFlag(Entity* entity, EntityFlag flag) { return entity->flags & flag; }

static void EntitySystemInit() { memset(&EntityM, 0, sizeof(EntityM)); }

static Entity* AllocateEntity() {
    EntityId id = NextEntityId();
    ptrdiff_t idx = hmgeti(EntityM.entities, id);
    bool first_frame = idx == -1;
    if (first_frame) {
        Entity entity = {0};
        entity.id = id;
        hmput(EntityM.entities, id, entity);
        arrput(EntityM.entity_ids, id);
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
    if (EntityTestFlag(entity, EFLAG_PLAYER_CONTROLLED)) {
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

    if (EntityTestFlag(entity, EFLAG_MOVE_RANDOM)) {
        static float cooldown = 0.0f;
        static const float time = 5.0f;
        cooldown += delta;
        if (cooldown >= time) {
            float min = 0.0f;
            float max = GetScreenWidth();
            float choice = GetRandomValue(min, max);
            entity->p.x = choice;

            cooldown = 0.0f;
        }
    }
}

static void EntityDraw(Entity* entity) {
    DrawTextureRec(entity->texture, entity->bitmap_rec, (Vector2){entity->p.x, entity->p.y}, WHITE);
}

////////////////////////////////////////////////////////////

typedef struct GameState {
    Camera camera;
} GameState;

static GameState GState;

static void SimpleScene(GameState* state) {
    float ground_y = 0.0f;

    {
        Entity* entity = AllocateEntity();
        float y = GetScreenHeight() * 0.65;
        ground_y = y;
        entity->p = (Vector3){0, y, 0};
        int width = GetScreenWidth();
        int height = y * 2.0f;
        entity->texture = LoadTextureFromImage(GenImageColor(width, height, GRAY));
        entity->bitmap_rec = (Rectangle){0, 0, width, height};
    }

    {
        Entity* entity = AllocateEntity();
        entity->flags = EFLAG_PLAYER_CONTROLLED;
        int width = 28;
        int height = 40;
        float y = ground_y - height;
        entity->p = (Vector3){GetScreenWidth() * 0.5, y, 0};
        entity->texture = LoadTextureFromImage(GenImageColor(width, height, BLUE));
        entity->bitmap_rec = (Rectangle){0, 0, width, height};
    }

    {
        Entity* entity = AllocateEntity();
        entity->flags = EFLAG_MOVE_RANDOM;
        float y = GetScreenHeight() * 0.10;
        entity->p = (Vector3){0, y, 0};
        int width = 50;
        int height = 25;
        entity->texture = LoadTextureFromImage(GenImageColor(width, height, RED));
        entity->bitmap_rec = (Rectangle){0, 0, width, height};
    }
}

static void GameStateInit(GameState* state) {
    memset(state, 0, sizeof(GameState));
    InitWindow(ScreenWidth, ScreenHeight, "Game");
    SetTargetFPS(60);
    EntitySystemInit();

    SimpleScene(state);
}

static void GameStateUpdate(GameState* state, float delta) {
    EntityManager* em = GetEntityManager();
    for (uint32_t i = 0; i < arrlenu(em->entity_ids); ++i) {
        Entity* entity = GetEntity(em->entity_ids[i]);
        EntityUpdate(entity, delta);
    }
}

static void GameStateDraw(GameState* state) {
    BeginDrawing();

    ClearBackground(RAYWHITE);

    EntityManager* em = GetEntityManager();
    for (uint32_t i = 0; i < arrlenu(em->entity_ids); ++i) {
        Entity* entity = GetEntity(em->entity_ids[i]);
        EntityDraw(entity);
    }

    EndDrawing();
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