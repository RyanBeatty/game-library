#ifndef UNITY_BUILD
#include <stdbool.h>

#include "entity.c"
#include "raymath.h"

#endif

static char* GameLevelsPath = "resources/levels";

typedef struct LevelData {
    char name[256];
    bool finished;
    bool failed;
    Vector3 start;
    Vector3 goal;
    Camera3D camera;
    // stbds array.
    Entity* entities;
} LevelData;

static LevelData CurLevel = {0};
#define GetCurLevel() (&CurLevel)

#define LevelDataNextEntityId(level_data) (arrlenu((level_data->entities)))

static Entity* LevelDataCreateEntity(LevelData* level_data) {
    uint32_t idx = LevelDataNextEntityId(level_data);
    Entity* entity = arraddnptr(level_data->entities, 1);
    memset(entity, 0, sizeof(Entity));
    entity->idx = idx;
    return entity;
}

static Entity* CreateLevelDataEntity(LevelData* level_data) {
    Entity* entity = LevelDataCreateEntity(level_data);
    return entity;
}

#define LevelDataGetStart(level_data) ((level_data)->start)

static Entity* LevelDataGetPlayer(LevelData* level_data) { return &level_data->entities[0]; }

static Entity* LevelDataGetEntity(LevelData* level_data, uint32_t i) { return &level_data->entities[i]; }

static uint32_t LevelDataNumEntities(LevelData* level_data) { return arrlenu(level_data->entities); }

static Entity* LevelDataGetGoal(LevelData* level_data) { return LevelDataGetEntity(level_data, 1); }

static void LevelDataDeleteEntity(LevelData* level_data, uint32_t index) {
    arrdel(level_data->entities, index);
    for (uint32_t i = 0; i < LevelDataNumEntities(level_data); ++i) {
        Entity* entity = LevelDataGetEntity(level_data, i);
        entity->idx = i;
    }
}

static char* LevelDataGetNextLevelName(LevelData* level) {
    uint32_t num = 0;
    int res = sscanf(level->name, "level_%d", &num);
    assert(res == 1);
    ++num;

    char* buffer = (char*)calloc(256, sizeof(char));
    sprintf(buffer, "level_%d", num);
    return buffer;
}

typedef struct RayCollisionLevelData {
    RayCollision collision;
    uint32_t entity_idx;
} RayCollisionLevelData;

static RayCollisionLevelData GetRayCollisionLevelData(Ray ray, LevelData* level_data) {
    RayCollisionLevelData collision = {0};
    for (uint32_t i = 0; i < LevelDataNumEntities(level_data); ++i) {
        Entity* entity = LevelDataGetEntity(level_data, i);
        RayCollision new_collision = GetRayCollisionEntity(ray, entity);
        if (!collision.collision.hit || (new_collision.hit && new_collision.distance < collision.collision.distance)) {
            collision.collision = new_collision;
            collision.entity_idx = entity->idx;
        }
    }
    return collision;
}
