#ifndef UNITY_BUILD
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "raylib.h"
#include "raymath.h"

#endif

const int ScreenWidth = 800;
const int ScreenHeight = 450;

static const Vector3 DOWN = {0, -1, 0};

////////////////////////////////////////////////////////////

typedef enum SoundId {
    SOUND_NONE = 0,
    SOUND_BOMB_EXPLOSION,
    SOUND_PICKUP_COIN,
    SOUND_FOOTSTEP_LEFT,
    SOUND_FOOTSTEP_RIGHT,
    SOUND_GAME_OVER,
    SOUND_GAME_START,
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
typedef enum EntityFlag {
    EFLAG_NONE = 0,
    EFLAG_ACTIVE = 1 << 0,
    EFLAG_PLAYER_CONTROLLED = 1 << 2,
    EFLAG_SPAWN_BOMB = 1 << 3,
    EFLAG_BOMB = 1 << 4,
    EFLAG_WALL = 1 << 5,
    EFLAG_EXPLOSION = 1 << 6,
    EFLAG_COIN = 1 << 7,
    EFLAG_DRAW = 1 << 8
} EntityFlag;

typedef uint32_t EntityId;

typedef struct Entity {
    EntityId id;
    Vector3 p;
    Vector3 pp;
    Vector3 a;
    float m;
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

    float damage_timer;
    float damage_cooldown;

    float dash_timer;

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

typedef struct GameState {
    Camera camera;

    uint16_t health;

    bool debug;
    bool game_over;

    uint32_t score;

    float timer;
} GameState;

static GameState GState;

#define GetGameState() (&GState)

static void SimpleScene(GameState* state) {
    EntitySystemInit();
    state->score = 0.0f;

    float ground_y = 0.0f;

    // Ground.
    {
        Entity* entity = AllocateEntity();
        SetFlag(entity->flags, EFLAG_WALL);
        float y = GetScreenHeight() * 0.65;
        ground_y = y;
        int width = GetScreenWidth();
        int height = y * 2.0f;
        entity->p = (Vector3){width * 0.5f, y + (height * 0.5f), 0};
        entity->pp = entity->p;
        entity->texture = LoadTextureFromImage(GenImageColor(width, height, GRAY));
        entity->bitmap_rec = (Rectangle){0, 0, width, height};
        entity->shape = (Shape){.kind = SHAPE_REC, .width = width, .height = height};
    }

    // Left wall.
    {
        Entity* entity = AllocateEntity();
        SetFlag(entity->flags, EFLAG_WALL);
        int width = 100;
        int height = GetScreenHeight();
        entity->p = (Vector3){-width + (width * 0.5f), height * 0.5f, 0};
        entity->pp = entity->p;
        entity->texture = LoadTextureFromImage(GenImageColor(width, height, BLANK));
        entity->bitmap_rec = (Rectangle){0, 0, width, height};
        entity->shape = (Shape){.kind = SHAPE_REC, .width = width, .height = height};
    }

    // Right wall.
    {
        Entity* entity = AllocateEntity();
        SetFlag(entity->flags, EFLAG_WALL);
        int width = 100;
        int height = GetScreenHeight();
        entity->p = (Vector3){GetScreenWidth() + (width * 0.5f), height * 0.5f, 0};
        entity->pp = entity->p;
        entity->texture = LoadTextureFromImage(GenImageColor(width, height, BLANK));
        entity->bitmap_rec = (Rectangle){0, 0, width, height};
        entity->shape = (Shape){.kind = SHAPE_REC, .width = width, .height = height};
    }

    // Player.
    {
        Entity* entity = AllocateEntity();
        SetFlag(entity->flags, EFLAG_PLAYER_CONTROLLED);
        int width = 28;
        int height = 40;
        float y = ground_y - height;
        entity->p = (Vector3){GetScreenWidth() * 0.5 + (width * 0.5f), y + (height * 0.5f), 0};
        entity->pp = entity->p;
        entity->texture = LoadTextureFromImage(GenImageColor(width, height, BLUE));
        entity->bitmap_rec = (Rectangle){0, 0, width, height};
        entity->shape = (Shape){.kind = SHAPE_REC, .width = width, .height = height};
        entity->damage_cooldown = 0.4f;
        entity->damage_timer = 0.0f;
    }

    // Bomb spawner.
    {
        Entity* entity = AllocateEntity();
        SetFlag(entity->flags, EFLAG_SPAWN_BOMB);
        float y = GetScreenHeight() * 0.10;
        int width = 50;
        int height = 25;
        entity->p = (Vector3){width * 0.5f, y + (height * 0.5f), 0};
        entity->pp = entity->p;
        entity->texture = LoadTextureFromImage(GenImageColor(width, height, RED));
        entity->bitmap_rec = (Rectangle){0, 0, width, height};
        entity->spawn_move_time = 2.5f;
        SetFlag(entity->spawn_flags, SFLAG_BOMB);
        FlagOff(entity->flags, EFLAG_DRAW);
    }

    // Coin spawner.
    {
        Entity* entity = AllocateEntity();
        SetFlag(entity->flags, EFLAG_SPAWN_BOMB);
        int width = 50;
        int height = 25;
        float y = ground_y - height - 10.0f;
        entity->p = (Vector3){width * 0.5f, y + (height * 0.5f), 0};
        entity->pp = entity->p;
        entity->texture = LoadTextureFromImage(GenImageColor(width, height, BLACK));
        entity->bitmap_rec = (Rectangle){0, 0, width, height};
        entity->spawn_move_time = 3.5f;
        SetFlag(entity->spawn_flags, SFLAG_COIN);
        FlagOff(entity->flags, EFLAG_DRAW);
    }

    state->health = 3;

    PlaySoundIfNotPlaying(SOUND_GAME_START);
}

static void GameStateInit(GameState* state) {
    memset(state, 0, sizeof(GameState));
    InitWindow(ScreenWidth, ScreenHeight, "Game");
    SetConfigFlags(FLAG_VSYNC_HINT);
    SetTargetFPS(60);
    EntitySystemInit();
    AudioSystemInit();

    SimpleScene(state);
}

static void GameStateUpdate(GameState* state, float delta) {
    state->timer += delta;

    if (IsSoundPlaying(GetSound(SOUND_GAME_START))) {
        return;
    }

    EntityManager* em = GetEntityManager();
    for (uint32_t i = 0; i < ArraySizeOf(em->entities); ++i) {
        Entity* entity = GetEntity(i);
        if (!TestFlag(entity->flags, EFLAG_ACTIVE)) {
            continue;
        }

        if (TestFlag(entity->flags, EFLAG_PLAYER_CONTROLLED)) {
            entity->damage_timer += delta;

            if (state->health == 0) {
                state->game_over = true;
                FlagOff(entity->flags, EFLAG_ACTIVE);
                PlaySoundIfNotPlaying(SOUND_GAME_OVER);
                continue;
            }

            Vector3 direction = Vector3Zero();
            if (IsKeyDown(KEY_RIGHT)) {
                direction = Vector3Add(direction, (Vector3){1, 0, 0});
            }
            if (IsKeyDown(KEY_LEFT)) {
                direction = Vector3Add(direction, (Vector3){-1, 0, 0});
            }

            float speed = 200.0f;
            entity->dash_timer += delta;
            if (IsKeyPressed(KEY_SPACE)) {
                entity->dash_timer = 0.0f;
                speed *= 30.0f;
            }
            Vector3 prev_p = entity->p;
            Vector3 movement = Vector3Scale(Vector3Scale(direction, speed), delta);
            entity->p = Vector3Add(entity->p, movement);

            bool walking = !Vector3Equals(movement, Vector3Zero());
            CollisionInfo info = CheckEntityCollisionScene(entity);
            for (uint32_t i = 0; i < info.len; ++i) {
                Entity* e = info.entities[i];
                if (TestFlag(e->flags, EFLAG_EXPLOSION) && entity->damage_timer >= entity->damage_cooldown) {
                    entity->damage_timer = 0.0f;
                    if (state->health > 0) {
                        --state->health;
                    }
                    walking = false;
                }
                if (TestFlag(e->flags, EFLAG_WALL)) {
                    entity->p = prev_p;
                    walking = false;
                }
                if (TestFlag(e->flags, EFLAG_COIN)) {
                    FlagOff(e->flags, EFLAG_ACTIVE);
                    ++state->score;
                    PlaySoundIfNotPlaying(SOUND_PICKUP_COIN);
                }
            }

            if (walking) {
                static const SoundId sounds[2] = {SOUND_FOOTSTEP_LEFT, SOUND_FOOTSTEP_RIGHT};
                static uint16_t sound_idx = 0;

                Sound walk_sound = GetSound(sounds[sound_idx]);
                if (!IsSoundPlaying(walk_sound)) {
                    sound_idx = (sound_idx + 1) % ArraySizeOf(sounds);
                    PlaySound(GetSound(sounds[sound_idx]));
                }
            }
        }

        if (TestFlag(entity->flags, EFLAG_SPAWN_BOMB)) {
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

                // Calculate random initial velocity.
                Vector2 v = Vector2Rotate((Vector2){1, 0}, GetRandomValue(0, 180) * DEG2RAD);
                v = Vector2Scale(v, GetRandomValue(1, 10));
                Vector3 v3 = {v.x, v.y, 0};

                // Spawn bomb.
                if (TestFlag(entity->spawn_flags, SFLAG_BOMB)) {
                    Entity* bomb = AllocateEntity();
                    SetFlag(bomb->flags, EFLAG_BOMB);
                    int width = 8;
                    int height = 15;
                    bomb->p = entity->p;
                    bomb->pp = Vector3Subtract(entity->p, Vector3Multiply(DOWN, (Vector3){20, 0, 0}));
                    bomb->texture = LoadTextureFromImage(GenImageColor(width, height, BLACK));
                    bomb->bitmap_rec = (Rectangle){0, 0, width, height};
                    bomb->shape = (Shape){.kind = SHAPE_REC, .width = width, .height = height};
                    bomb->m = 3.0f;
                }

                // Spawn Coin.
                if (TestFlag(entity->spawn_flags, SFLAG_COIN)) {
                    Entity* bomb = AllocateEntity();
                    SetFlag(bomb->flags, EFLAG_COIN);
                    bomb->p = entity->p;
                    int width = 20;
                    int height = 20;
                    bomb->texture = LoadTextureFromImage(GenImageColor(width, height, PINK));
                    bomb->bitmap_rec = (Rectangle){0, 0, width, height};
                    bomb->shape = (Shape){.kind = SHAPE_REC, .width = width, .height = height};
                }
            }
        }

        if (TestFlag(entity->flags, EFLAG_BOMB)) {
            entity->bomb_timer += delta;
            if (entity->bomb_timer >= 4.0f) {
                // Spawn explosion.
                {
                    Entity* explosion = AllocateEntity();
                    SetFlag(explosion->flags, EFLAG_EXPLOSION);
                    explosion->p = entity->p;
                    explosion->pp = explosion->p;
                    int width = 100;
                    int height = 100;
                    Image image = GenImageColor(width, height, BLANK);
                    Color color = YELLOW;
                    color.a = 125;
                    ImageDrawCircle(&image, width * 0.5f, height * 0.5f, width * 0.5f, color);
                    explosion->texture = LoadTextureFromImage(image);
                    explosion->bitmap_rec = (Rectangle){0, 0, width, height};
                    explosion->shape = (Shape){.kind = SHAPE_CIRCLE, .radius = width * 0.5f};
                }
                FlagOff(entity->flags, EFLAG_ACTIVE);
                continue;
            }

            // TODO: Once camera is set up, start in camera space and convert to screen space.
            const Vector3 g = {0, -9.8 * 1000.0f, 0};
            Vector3 force = Vector3Scale(g, entity->m);
            entity->a = Vector3Scale(force, 1.0f / entity->m);
            Vector3 displacement =
                Vector3Add(Vector3Subtract(entity->p, entity->pp), Vector3Scale(entity->a, delta * delta));
            entity->p = Vector3Add(entity->p, Vector3Multiply(DOWN, displacement));

            CollisionInfo info = CheckEntityCollisionScene(entity);
            for (uint32_t i = 0; i < info.len; ++i) {
                Entity* e = info.entities[i];
                if (TestFlag(e->flags, EFLAG_WALL)) {
                    entity->p = entity->pp;
                    entity->a = Vector3Zero();
                }
            }
        }

        if (TestFlag(entity->flags, EFLAG_EXPLOSION)) {
            static const float TIME = 0.2f;
            entity->explosion_timer += delta;
            if (entity->explosion_timer >= TIME) {
                FlagOff(entity->flags, EFLAG_ACTIVE);
                continue;
            }

            // Explosion shading.
            Color tint = {Lerp(BLACK.r, WHITE.r, entity->explosion_timer / TIME),
                          Lerp(BLACK.g, WHITE.g, entity->explosion_timer / TIME),
                          Lerp(BLACK.b, WHITE.b, entity->explosion_timer / TIME), 255};
            entity->tint = tint;

            PlaySoundIfNotPlaying(SOUND_BOMB_EXPLOSION);
        }

        entity->pp = entity->p;
    }
}

static void GameStateDraw(GameState* state, float delta) {
    BeginDrawing();

    state->debug ^= (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_D));
    if (state->debug) {
        DrawFPS(5, 5);
    }

    ClearBackground(RAYWHITE);

    EntityManager* em = GetEntityManager();
    for (uint32_t i = 0; i < ArraySizeOf(em->entities); ++i) {
        Entity* entity = GetEntity(i);
        if (!TestFlag(entity->flags, EFLAG_ACTIVE) || !TestFlag(entity->flags, EFLAG_DRAW)) {
            continue;
        }
        Color tint = entity->tint;
        if (entity->damage_timer < entity->damage_cooldown) {
            tint.a = Lerp(tint.a, 0, sin(state->timer * 100.0f));
        }

        float half_width = entity->bitmap_rec.width * 0.5f;
        float half_height = entity->bitmap_rec.height * 0.5f;
        DrawTextureRec(entity->texture, entity->bitmap_rec,
                       (Vector2){entity->p.x - half_width, entity->p.y - half_height}, tint);
    }

    // Draw UI.
    {
        {
            char health_text[256] = {0};
            memset(health_text, '*', sizeof(char) * state->health);
            float fontsize = 24.0f;
            float x = 100.0f;
            float y = 20.0f;
            DrawText(health_text, x, y, fontsize, BLACK);
        }

        {
            char score_text[256] = {0};
            sprintf(score_text, "%d", state->score);
            float fontsize = 24.0f;
            float x = GetScreenWidth() - 100.0f;
            float y = 20.0f;
            DrawText(score_text, x, y, fontsize, BLACK);
        }

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
        float delta = GetFrameTime();
        GameStateUpdate(&GState, delta);
        GameStateDraw(&GState, delta);
    }

    CloseWindow();

    return 0;
}