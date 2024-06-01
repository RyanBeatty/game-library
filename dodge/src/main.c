#ifndef UNITY_BUILD
#include <stdlib.h>
#include <time.h>

#include "raylib.h"
#include "raymath.h"

#endif

const int ScreenWidth = 800;
const int ScreenHeight = 450;

////////////////////////////////////////////////////////////

typedef enum ShapeKind { SHAPE_NONE = 0, SHAPE_RECT, SHAPE_CIRCLE, SHAPE_COUNT } ShapeKind;

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

#define Shape2Rec(c, s) ((Rectangle){(c).x, (c).y, (s).width, (s).height})

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
        case SHAPE_RECT: {
            switch (s2.kind) {
                case SHAPE_RECT: {
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
                case SHAPE_RECT: {
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

    float spawn_move_timer;
    float spawn_move_time;
    SpawnFlag spawn_flags;

    // TODO: Consider cooldown?
    float dash_timer;
    int last_key;

    Shape shape;
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
// Collision.

typedef struct CollisionInfo {
    // bool dead;
    // bool stopped;
    Entity* entities[ENTITY_CAP];
    uint32_t len;
} CollisionInfo;

static CollisionInfo CheckEntityCollisionScene(Entity* entity) {
    Vector2 e1p = {entity->p.x, entity->p.y};
    EntityManager* em = GetEntityManager();
    CollisionInfo info = {0};
    for (uint32_t i = 0; i < ArraySizeOf(em->entities); ++i) {
        Entity* e = GetEntity(i);
        if (!EntityTestFlag(e, EFLAG_ACTIVE) || e->id == entity->id) {
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

typedef struct GameState {
    Camera camera;

    bool debug;
    bool game_over;
} GameState;

static GameState GState;

#define GetGameState() (&GState)

static void SimpleScene(GameState* state) {
    EntitySystemInit();

    float ground_y = 0.0f;

    // Ground.
    {
        Entity* entity = AllocateEntity();
        SetFlag(entity->flags, EFLAG_WALL);
        float y = GetScreenHeight() * 0.65;
        ground_y = y;
        entity->p = (Vector3){0, y, 0};
        int width = GetScreenWidth();
        int height = y * 2.0f;
        entity->texture = LoadTextureFromImage(GenImageColor(width, height, GRAY));
        entity->bitmap_rec = (Rectangle){0, 0, width, height};
        entity->shape = (Shape){.kind = SHAPE_RECT, .width = width, .height = height};
    }

    // Left wall.
    {
        Entity* entity = AllocateEntity();
        SetFlag(entity->flags, EFLAG_WALL);
        int width = 100;
        int height = GetScreenHeight();
        entity->p = (Vector3){-width, 0, 0};
        entity->texture = LoadTextureFromImage(GenImageColor(width, height, BLANK));
        entity->bitmap_rec = (Rectangle){0, 0, width, height};
        entity->shape = (Shape){.kind = SHAPE_RECT, .width = width, .height = height};
    }

    // Right wall.
    {
        Entity* entity = AllocateEntity();
        SetFlag(entity->flags, EFLAG_WALL);
        int width = 100;
        int height = GetScreenHeight();
        entity->p = (Vector3){GetScreenWidth(), 0, 0};
        entity->texture = LoadTextureFromImage(GenImageColor(width, height, BLANK));
        entity->bitmap_rec = (Rectangle){0, 0, width, height};
        entity->shape = (Shape){.kind = SHAPE_RECT, .width = width, .height = height};
    }

    // Player.
    {
        Entity* entity = AllocateEntity();
        SetFlag(entity->flags, EFLAG_PLAYER_CONTROLLED);
        int width = 28;
        int height = 40;
        float y = ground_y - height;
        entity->p = (Vector3){GetScreenWidth() * 0.5, y, 0};
        entity->texture = LoadTextureFromImage(GenImageColor(width, height, BLUE));
        entity->bitmap_rec = (Rectangle){0, 0, width, height};
        entity->shape = (Shape){.kind = SHAPE_RECT, .width = width, .height = height};
    }

    // Bomb spawner.
    {
        Entity* entity = AllocateEntity();
        SetFlag(entity->flags, EFLAG_SPAWN_BOMB);
        float y = GetScreenHeight() * 0.10;
        entity->p = (Vector3){0, y, 0};
        int width = 50;
        int height = 25;
        entity->texture = LoadTextureFromImage(GenImageColor(width, height, RED));
        entity->bitmap_rec = (Rectangle){0, 0, width, height};
        entity->spawn_move_time = 2.5f;
        SetFlag(entity->spawn_flags, SFLAG_BOMB);
    }

    // Coin spawner.
    {
        Entity* entity = AllocateEntity();
        SetFlag(entity->flags, EFLAG_SPAWN_BOMB);
        int width = 50;
        int height = 25;
        float y = ground_y - height - 10.0f;
        entity->p = (Vector3){0, y, 0};
        entity->texture = LoadTextureFromImage(GenImageColor(width, height, BLACK));
        entity->bitmap_rec = (Rectangle){0, 0, width, height};
        entity->spawn_move_time = 3.5f;
        SetFlag(entity->spawn_flags, SFLAG_COIN);
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
            Vector3 prev_p = entity->p;
            entity->p = Vector3Add(entity->p, Vector3Scale(Vector3Scale(direction, speed), delta));

            CollisionInfo info = CheckEntityCollisionScene(entity);
            for (uint32_t i = 0; i < info.len; ++i) {
                Entity* e = info.entities[i];
                if (TestFlag(e->flags, EFLAG_EXPLOSION)) {
                    state->game_over = true;
                    entity->flags &= ~EFLAG_ACTIVE;
                }
                if (TestFlag(e->flags, EFLAG_WALL)) {
                    entity->p = prev_p;
                }
            }
        }

        if (EntityTestFlag(entity, EFLAG_SPAWN_BOMB)) {
            entity->spawn_move_timer += delta;
            if (entity->spawn_move_timer >= entity->spawn_move_time) {
                entity->spawn_move_timer = 0.0f;
                // Move randomly.
                {
                    float min = 0.0f;
                    float max = GetScreenWidth();
                    float choice = GetRandomValue(min, max);
                    entity->p.x = choice;
                }

                // Spawn bomb.
                if (TestFlag(entity->spawn_flags, SFLAG_BOMB)) {
                    Entity* bomb = AllocateEntity();
                    SetFlag(bomb->flags, EFLAG_BOMB);
                    bomb->p = entity->p;
                    int width = 8;
                    int height = 15;
                    bomb->texture = LoadTextureFromImage(GenImageColor(width, height, BLACK));
                    bomb->bitmap_rec = (Rectangle){0, 0, width, height};
                    bomb->shape = (Shape){.kind = SHAPE_RECT, .width = width, .height = height};
                }
            }
        }

        if (EntityTestFlag(entity, EFLAG_BOMB)) {
            entity->bomb_timer += delta;
            if (entity->bomb_timer >= 4.0f) {
                // Spawn explosion.
                {
                    Entity* explosion = AllocateEntity();
                    SetFlag(explosion->flags, EFLAG_EXPLOSION);
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
                    explosion->shape = (Shape){.kind = SHAPE_CIRCLE, .radius = width * 0.5f};
                }
                entity->flags &= ~EFLAG_ACTIVE;  // disable bomb entity.
                continue;
            }

            // TODO: Once camera is set up, start in camera space and convert to screen space.
            Vector3 dir = {0, 1, 0};
            Vector3 movement = Vector3Scale(dir, 100 * delta);
            Vector3 prev_p = entity->p;
            entity->p = Vector3Add(entity->p, movement);

            CollisionInfo info = CheckEntityCollisionScene(entity);
            for (uint32_t i = 0; i < info.len; ++i) {
                Entity* e = info.entities[i];
                if (TestFlag(e->flags, EFLAG_WALL)) {
                    entity->p = prev_p;
                }
            }
        }

        if (EntityTestFlag(entity, EFLAG_EXPLOSION)) {
            static const float TIME = 0.2f;
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

    // Draw UI.
    {
        if (state->game_over) {
            static const char* GAME_OVER_TEXT = "Game Over";
            static const char* RETRY_TEXT = "Press Enter to Retry";

            Color c = BLACK;
            c.a *= 0.40f;
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), c);

            Font font = GetFontDefault();
            float fontsize = 24.0f;
            Vector2 size = MeasureTextEx(font, GAME_OVER_TEXT, fontsize, fontsize / font.baseSize);
            float y = (GetScreenHeight() - size.y) * 0.5f;
            DrawText(GAME_OVER_TEXT, (GetScreenWidth() - size.x) * 0.5f, y, fontsize, WHITE);
            y += size.y + 5.0f;
            fontsize = 18.0f;
            size = MeasureTextEx(font, RETRY_TEXT, fontsize, fontsize / font.baseSize);
            DrawText(RETRY_TEXT, (GetScreenWidth() - size.x) * 0.5f, y, fontsize, WHITE);

            if (IsKeyDown(KEY_ENTER)) {
                state->game_over = false;
                // TODO: Should probably move this somewhere else.
                SimpleScene(state);
            }
        }
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