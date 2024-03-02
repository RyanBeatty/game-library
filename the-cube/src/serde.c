#ifndef UNITY_BUILD
#include <stdint.h>

#include "entity.c"
#include "level.c"
#include "raylib.h"

#endif

static char* PackDelim(char* buffer) {
    int i = sprintf(buffer, ",");
    assert(i != -1);
    return &buffer[i];
}

static char* PackFloat(char* buffer, float f) {
    int i = sprintf(buffer, "%f", f);
    return &buffer[i];
}

static char* PackU8(char* buffer, uint8_t b) {
    int i = sprintf(buffer, "%hhu", b);
    return &buffer[i];
}

static char* PackU32(char* buffer, uint32_t x) {
    int i = sprintf(buffer, "%u", x);
    return &buffer[i];
}

static char* PackColor(char* buffer, Color c) {
    buffer = PackU8(buffer, c.r);
    buffer = PackDelim(buffer);
    buffer = PackU8(buffer, c.g);
    buffer = PackDelim(buffer);
    buffer = PackU8(buffer, c.b);
    buffer = PackDelim(buffer);
    buffer = PackU8(buffer, c.a);
    return buffer;
}

static char* PackVector3(char* buffer, Vector3 v) {
    buffer = PackFloat(buffer, v.x);
    buffer = PackDelim(buffer);
    buffer = PackFloat(buffer, v.y);
    buffer = PackDelim(buffer);
    buffer = PackFloat(buffer, v.z);
    return buffer;
}

static char* PackVector4(char* buffer, Vector4 v) {
    buffer = PackFloat(buffer, v.x);
    buffer = PackDelim(buffer);
    buffer = PackFloat(buffer, v.y);
    buffer = PackDelim(buffer);
    buffer = PackFloat(buffer, v.z);
    buffer = PackDelim(buffer);
    buffer = PackFloat(buffer, v.w);
    return buffer;
}

static char* PackQuaternion(char* buffer, Quaternion q) { return PackVector4(buffer, q); }

static char* PackFlags(char* buffer, uint8_t flags[FLAGS_LEN]) {
    buffer = PackU32(buffer, FLAGS_LEN);
    buffer = PackDelim(buffer);
    for (uint32_t i = 0; i < FLAGS_LEN; ++i) {
        buffer = PackU8(buffer, flags[i]);
        if (i < FLAGS_LEN - 1) {
            buffer = PackDelim(buffer);
        }
    }
    return buffer;
}

static char* WriteBuffer(char* data, char* to_write, size_t len) {
    for (uint32_t i = 0; i < len; ++i) {
        arrpush(data, to_write[i]);
    }
    arrpush(data, '\n');
    return data;
}

static void SaveLevel(LevelData* level_data) {
    char* data = NULL;

    level_data->start = LevelDataGetPlayer(level_data)->position;
    level_data->goal = LevelDataGetGoal(level_data)->position;

    {
        char buffer[256] = {0};
        PackVector3(buffer, level_data->start);
        data = WriteBuffer(data, buffer, strlen(buffer));
    }
    {
        // Write goal position instead of what is in level data in case we changed the position of goal entity.
        Entity* goal = LevelDataGetGoal(level_data);
        char buffer[256] = {0};
        PackVector3(buffer, goal->position);
        data = WriteBuffer(data, buffer, strlen(buffer));
    }

    // Pack entities.
    for (uint32_t i = 0; i < LevelDataNumEntities(level_data); ++i) {
        Entity* entity = LevelDataGetEntity(level_data, i);
        if (entity == LevelDataGetPlayer(level_data)) {
            continue;
        }
        if (entity == LevelDataGetGoal(level_data)) {
            continue;
        }

        char buffer[1024] = {0};
        char* pos = buffer;
        int ii = sprintf(pos, "%u", entity->idx);
        pos = &buffer[ii];
        pos = PackDelim(pos);
        pos = PackVector3(pos, entity->position);
        pos = PackDelim(pos);
        pos = PackQuaternion(pos, entity->orientation);
        pos = PackDelim(pos);
        pos = PackColor(pos, entity->block_color);
        pos = PackDelim(pos);
        pos = PackFlags(pos, entity->flags);

        data = WriteBuffer(data, buffer, strlen(buffer));
    }

    arrpush(data, '\0');

    char filepath[1024];
    sprintf(filepath, "%s/%s", GameLevelsPath, level_data->name);
    SaveFileText(filepath, data);
}

static char* GetNextToken(char** buffer) { return strtok_r(NULL, ",", buffer); }

static char* UnpackU32(char* buffer, uint32_t* idx) {
    char* tok = GetNextToken(&buffer);
    int i = sscanf(tok, "%u", idx);
    assert(i != -1);
    return buffer;
}

static char* UnpackFloat(char* buffer, float* f) {
    char* tok = GetNextToken(&buffer);
    int i = sscanf(tok, "%f", f);
    assert(i != -1);
    return buffer;
}

static char* UnpackU8(char* buffer, uint8_t* b) {
    char* tok = GetNextToken(&buffer);
    int i = sscanf(tok, "%hhu", b);
    assert(i != -1);
    return buffer;
}

static char* UnpackVector3(char* buffer, Vector3* v) {
    buffer = UnpackFloat(buffer, &v->x);
    buffer = UnpackFloat(buffer, &v->y);
    buffer = UnpackFloat(buffer, &v->z);
    return buffer;
}

static char* UnpackVector4(char* buffer, Vector4* v) {
    buffer = UnpackFloat(buffer, &v->x);
    buffer = UnpackFloat(buffer, &v->y);
    buffer = UnpackFloat(buffer, &v->z);
    buffer = UnpackFloat(buffer, &v->w);
    return buffer;
}

static char* UnpackQuaternion(char* buffer, Quaternion* q) { return UnpackVector4(buffer, q); }

static char* UnpackColor(char* buffer, Color* c) {
    buffer = UnpackU8(buffer, &c->r);
    buffer = UnpackU8(buffer, &c->g);
    buffer = UnpackU8(buffer, &c->b);
    buffer = UnpackU8(buffer, &c->a);
    return buffer;
}

static char* UnpackFlags(char* buffer, uint8_t flags[FLAGS_LEN]) {
    uint32_t len = 0;
    buffer = UnpackU32(buffer, &len);
    assert(len <= FLAGS_LEN);
    for (uint32_t i = 0; i < len; ++i) {
        uint8_t flag = 0;
        buffer = UnpackU8(buffer, &flag);
        flags[i] = flag;
    }
    return buffer;
}

static void LoadLevel(const char* name_original) {
    // We make this dumb copy here because we will run into issues where when we clear CurLevel down below,
    // name_original might be wiped if we passed in the pointer to CurLevel.name.
    char name[256] = {0};
    strcpy(name, name_original);

    char filepath[1024];
    sprintf(filepath, "%s/%s", GameLevelsPath, name);
    char* data = LoadFileText(filepath);

    CurLevel = (LevelData){0};
    LevelData* level_data = &CurLevel;  // Do this so we don't lose the stbarry entity info.
    strcpy(level_data->name, name);

    // Reset Player/entities.
    Entity* player = LevelDataCreateEntity(level_data);

    char* line = strtok(data, "\n");
    UnpackVector3(line, &level_data->start);
    line = strtok(NULL, "\n");
    PlayerInit(player, level_data->start);

    UnpackVector3(line, &level_data->goal);
    line = strtok(NULL, "\n");

    Entity* goal = CreateLevelDataEntity(level_data);
    goal->orientation = QuaternionIdentity();
    goal->model = GenCubeModel(GREEN);
    goal->position = level_data->goal;
    EntitySetFlag(goal, EF_LEVEL_SUCCESS_ON_COLLISION);

    // Unpack entities.
    while (line != NULL) {
        // Start buffer at the beginning of line, this is just so we have line around in case debuggin is needed.
        char* buffer = line;

        Entity* entity = CreateLevelDataEntity(level_data);

        buffer = UnpackU32(buffer, &entity->idx);
        buffer = UnpackVector3(buffer, &entity->position);
        buffer = UnpackQuaternion(buffer, &entity->orientation);
        buffer = UnpackColor(buffer, &entity->block_color);
        buffer = UnpackFlags(buffer, entity->flags);

        // TODO: Make this better. Come up with way to store model info on each entity.
        if (EntityTestFlag(entity, EF_HUNT_PLAYER)) {
            entity->model = GenHunterModel(entity->block_color);
            // TODO: improve this. just for visual debugging at the moment.
            entity->position.y = 0;
        } else {
            entity->model = GenCubeModel(entity->block_color);
        }

        line = strtok(NULL, "\n");
    }
}