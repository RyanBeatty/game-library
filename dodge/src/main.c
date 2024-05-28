#ifndef UNITY_BUILD
#include <stdlib.h>
#include <time.h>

#include "raylib.h"
#include "raymath.h"

#endif

const int ScreenWidth = 800;
const int ScreenHeight = 450;

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
typedef enum EntityFlag {
    EFLAG_NONE = 0,
    EFLAG_ACTIVE = 1 << 0,
    EFLAG_PLAYER_CONTROLLED = 1 << 2,
    EFLAG_SPAWN_BOMB = 1 << 3,
    EFLAG_BOMB = 1 << 4
} EntityFlag;

typedef uint32_t EntityId;

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

#define ENTITY_CAP 256

typedef struct EntityManager {
    Entity entities[ENTITY_CAP];
} EntityManager;

static EntityManager EntityM = {0};

#define GetEntityManager() &EntityM

static bool EntityTestFlag(Entity* entity, EntityFlag flag) { return entity->flags & flag; }

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
        if (!EntityTestFlag(entity, EFLAG_ACTIVE)) {
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
    return entity;
}

// static void EntityUpdate(Entity* entity, float delta) {}

static void EntityDraw(Entity* entity) {
    DrawTextureRec(entity->texture, entity->bitmap_rec, (Vector2){entity->p.x, entity->p.y}, WHITE);
}

////////////////////////////////////////////////////////////

typedef struct GameState {
    Camera camera;
} GameState;

static GameState GState;

#define GetGameState() (&GState)

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
        entity->flags |= EFLAG_PLAYER_CONTROLLED;
        int width = 28;
        int height = 40;
        float y = ground_y - height;
        entity->p = (Vector3){GetScreenWidth() * 0.5, y, 0};
        entity->texture = LoadTextureFromImage(GenImageColor(width, height, BLUE));
        entity->bitmap_rec = (Rectangle){0, 0, width, height};
    }

    {
        Entity* entity = AllocateEntity();
        entity->flags |= EFLAG_SPAWN_BOMB;
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
    for (uint32_t i = 0; i < ArraySizeOf(em->entities); ++i) {
        Entity* entity = GetEntity(i);
        if (!EntityTestFlag(entity, EFLAG_ACTIVE)) {
            continue;
        }

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

        if (EntityTestFlag(entity, EFLAG_SPAWN_BOMB)) {
            static float cooldown = 0.0f;
            static const float time = 2.5f;
            cooldown += delta;
            if (cooldown >= time) {
                // Move randomly.
                {
                    float min = 0.0f;
                    float max = GetScreenWidth();
                    float choice = GetRandomValue(min, max);
                    entity->p.x = choice;
                }

                // Spawn bomb.
                {
                    Entity* bomb = AllocateEntity();
                    bomb->flags |= EFLAG_BOMB;
                    bomb->p = entity->p;
                    int width = 8;
                    int height = 15;
                    bomb->texture = LoadTextureFromImage(GenImageColor(width, height, BLACK));
                    bomb->bitmap_rec = (Rectangle){0, 0, width, height};
                }

                cooldown = 0.0f;
            }
        }

        if (EntityTestFlag(entity, EFLAG_BOMB)) {
            // Vector3 dir_camera = {0, -1, 0};
            // Matrix view = GetCameraMatrix2D(GetGameState()->camera);
            // Vector3 movement = Vector3Scale(dir, 1);
            // entity->p = Vector3Add(entity->p, movement);
        }
    }
}

static void GameStateDraw(GameState* state) {
    BeginDrawing();

    ClearBackground(RAYWHITE);

    EntityManager* em = GetEntityManager();
    for (uint32_t i = 0; i < ArraySizeOf(em->entities); ++i) {
        Entity* entity = GetEntity(i);
        if (!EntityTestFlag(entity, EFLAG_ACTIVE)) {
            continue;
        }
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