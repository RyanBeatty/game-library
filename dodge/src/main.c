#ifndef UNITY_BUILD
#include <stdlib.h>
#include <time.h>

#include "raylib.h"
#include "raymath.h"

#endif

const int ScreenWidth = 800;
const int ScreenHeight = 450;

////////////////////////////////////////////////////////////
typedef enum EntityFlag {
    EFLAG_NONE = 0,
    EFLAG_ACTIVE = 1 << 0,
    EFLAG_PLAYER_CONTROLLED = 1 << 2,
    EFLAG_SPAWN_BOMB = 1 << 3,
    EFLAG_BOMB = 1 << 4,
    EFLAG_WALL = 1 << 5,
    EFLAG_EXPLOSION = 1 << 6
} EntityFlag;

typedef uint32_t EntityId;

typedef struct Entity {
    EntityId id;
    Vector3 p;
    Quaternion rot;
    EntityFlag flags;

    Texture texture;
    Rectangle bitmap_rec;
    Color tint;

    float bomb_timer;
    float explosion_timer;

    // TODO: Consider cooldown?
    float dash_timer;
    int last_key;
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
    entity->tint = WHITE;
    return entity;
}

// static void EntityUpdate(Entity* entity, float delta) {}

static void EntityDraw(Entity* entity) {
    DrawTextureRec(entity->texture, entity->bitmap_rec, (Vector2){entity->p.x, entity->p.y}, entity->tint);
}

////////////////////////////////////////////////////////////

typedef struct GameState {
    Camera camera;

    bool debug;
} GameState;

static GameState GState;

#define GetGameState() (&GState)

static void SimpleScene(GameState* state) {
    float ground_y = 0.0f;

    {
        Entity* entity = AllocateEntity();
        entity->flags |= EFLAG_WALL;
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
    SetConfigFlags(FLAG_VSYNC_HINT);
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
            static const float DASH_WINDOW = 0.2f;

            Vector3 direction = Vector3Zero();
            if (IsKeyDown(KEY_RIGHT)) {
                direction = Vector3Add(direction, (Vector3){1, 0, 0});
            }
            if (IsKeyDown(KEY_LEFT)) {
                direction = Vector3Add(direction, (Vector3){-1, 0, 0});
            }

            float speed = 200.0f;
            entity->dash_timer += delta;
            int keys[2] = {KEY_LEFT, KEY_RIGHT};
            for (uint32_t i = 0; i < ArraySizeOf(keys); ++i) {
                int key = keys[i];
                if (IsKeyPressed(key)) {
                    if (entity->last_key == key && entity->dash_timer <= DASH_WINDOW) {
                        speed *= 30.0f;
                    }
                    entity->dash_timer = 0.0f;
                    entity->last_key = keys[i];
                }
            }
            entity->p = Vector3Add(entity->p, Vector3Scale(Vector3Scale(direction, speed), delta));

            // TODO: Refactor all of this.
            EntityManager* em = GetEntityManager();
            bool hit = false;
            for (uint32_t i = 0; i < ArraySizeOf(em->entities); ++i) {
                Entity* e = GetEntity(i);
                if (!EntityTestFlag(e, EFLAG_ACTIVE) || !EntityTestFlag(e, EFLAG_EXPLOSION) || e->id == entity->id) {
                    continue;
                }

                Rectangle rec = {entity->p.x, entity->p.y, entity->bitmap_rec.width, entity->bitmap_rec.height};
                Vector2 center = {e->p.x + (e->bitmap_rec.width * 0.5f), e->p.y + (e->bitmap_rec.height * 0.5f)};
                float radius = e->bitmap_rec.width * 0.5f;
                if (CheckCollisionCircleRec(center, radius, rec)) {
                    hit = true;
                    break;
                }
            }

            if (hit) {
                entity->flags &= ~EFLAG_ACTIVE;
                continue;
            }
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
            entity->bomb_timer += delta;
            if (entity->bomb_timer >= 4.0f) {
                // Spawn explosion.
                {
                    Entity* explosion = AllocateEntity();
                    explosion->flags |= EFLAG_EXPLOSION;
                    explosion->p = entity->p;
                    int width = 100;
                    int height = 100;
                    Image image = GenImageColor(width, height, BLANK);
                    Color color = YELLOW;
                    color.a = 125;
                    ImageDrawCircle(&image, width * 0.5f, height * 0.5f, width * 0.5f, color);
                    explosion->texture = LoadTextureFromImage(image);
                    explosion->bitmap_rec = (Rectangle){0, 0, width, height};
                    explosion->p.x -= width * 0.5;
                    explosion->p.y -= height * 0.5;
                }
                entity->flags &= ~EFLAG_ACTIVE;  // disable bomb entity.
                continue;
            }

            // TODO: Once camera is set up, start in camera space and convert to screen space.
            Vector3 dir = {0, 1, 0};
            Vector3 movement = Vector3Scale(dir, 100 * delta);
            Vector3 prev_p = entity->p;
            entity->p = Vector3Add(entity->p, movement);

            // TODO: Refactor all of this.
            EntityManager* em = GetEntityManager();
            bool hit = false;
            for (uint32_t i = 0; i < ArraySizeOf(em->entities); ++i) {
                Entity* e = GetEntity(i);
                if (!EntityTestFlag(e, EFLAG_ACTIVE) || !EntityTestFlag(e, EFLAG_WALL) || e->id == entity->id) {
                    continue;
                }

                Rectangle rec = {entity->p.x, entity->p.y, entity->bitmap_rec.width, entity->bitmap_rec.height};
                Rectangle rec2 = {e->p.x, e->p.y, e->bitmap_rec.width, e->bitmap_rec.height};
                if (CheckCollisionRecs(rec, rec2)) {
                    hit = true;
                    break;
                }
            }

            if (hit) {
                entity->p = prev_p;
            }
        }

        if (EntityTestFlag(entity, EFLAG_EXPLOSION)) {
            static const float TIME = 1.0f;
            entity->explosion_timer += delta;
            if (entity->explosion_timer >= TIME) {
                entity->flags &= ~EFLAG_ACTIVE;
                continue;
            }

            // Explosion shading.
            Color tint = {Lerp(BLACK.r, WHITE.r, entity->explosion_timer / TIME),
                          Lerp(BLACK.g, WHITE.g, entity->explosion_timer / TIME),
                          Lerp(BLACK.b, WHITE.b, entity->explosion_timer / TIME), 255};
            entity->tint = tint;
        }
    }
}

static void GameStateDraw(GameState* state) {
    BeginDrawing();

    state->debug ^= (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_D));
    if (state->debug) {
        DrawFPS(5, 5);
    }

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