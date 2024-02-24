
#ifndef UNITY_BUILD
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "entity.c"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#endif

/////////////////////////////////////////////////////////////////////////////////////
static Camera3D GameCamera;

void GameCameraInit(Camera3D* camera) {
    camera->position = (Vector3){0.0f, 20.0f, 20.0f};
    camera->target = (Vector3){0.0f, 0.0f, 0.0f};
    camera->up = (Vector3){0.0f, 1.0f, 0.0f};
    camera->fovy = 45.0f;
    camera->projection = CAMERA_PERSPECTIVE;
}

/////////////////////////////////////////////////////////////////////////////////////
// Non-Playable.

static void EntityLevelSuccessOnCollision(Entity* entity, float delta_time) {
    // Check for collision with goal.
    // TODO: Maybe rethink this since this causes problems depending on the size of the bounding box. for
    // example, if we make the bounding box the same size as cube, then we will accidentally collide if we move
    // to a square next to goal. Maybe instead we should convert entity positions to a "grid space" where each
    // grid spot has a unique id and use that to check collisions.
    Entity* player = LevelDataGetPlayer(&CurLevel);
    BoundingBox player_bb = EntityGetBoundingBox(player);
    BoundingBox goal_bb = EntityGetBoundingBox(entity);
    if (CheckCollisionBoxes(player_bb, goal_bb)) {
        CurLevel.finished = true;
    }
}

static void EntityTransparentOnCollision(Entity* entity, float delta_time) {
    Entity* player = LevelDataGetPlayer(&CurLevel);
    BoundingBox e_bb = GetBoundingBox(entity->position);
    BoundingBox p_bb = GetBoundingBox(player->position);

    unsigned char alpha = 255;
    if (CheckCollisionBoxes(e_bb, p_bb)) {
        Color p_color = entity->block_color;
        Color e_color = entity->block_color;
        if ((e_color.r == p_color.r || e_color.g == p_color.g || e_color.b == p_color.b)) {
            alpha = 255 / 2;
        } else {
            alpha = 255;
        }
    }
    entity->model.materials[0].maps[MATERIAL_MAP_DIFFUSE].color.a = alpha;
}

// TODO: Fully implement this once I shift grid to be on
static void EntityHuntPlayer(Entity* entity, float delta_time) {
    Entity* player = LevelDataGetPlayer(&CurLevel);

    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_DOWN)) {
        struct {
            Vector3 key;
            Vector3 value;
        }* parents = NULL;
        Vector3 end = World2Grid(player->position);
        Vector3 start = World2Grid(entity->position);
        hmput(parents, start, start);

        Vector3 queue[128];
        uint32_t head = 0;
        uint32_t tail = 1;
        queue[head] = start;

        // BFS to find next move.
        for (; head < tail; ++head) {
            Vector3 curr = queue[head];
            if (Vector3Equals(curr, end)) {
                break;
            }

            // Remember, everything in grid coords here so offsets all 1 instead of cube size.
            Vector3 dirs[4] = {{0, 0, -1}, {0, 0, 1}, {-1, 0, 0}, {1, 0, 0}};
            for (uint32_t i = 0; i < 4; ++i) {
                Vector3 child = Vector3Add(curr, dirs[i]);
                if (hmgeti(parents, child) == -1) {
                    bool hit = false;
                    for (uint32_t k = 0; k < LevelDataNumEntities(&CurLevel); ++k) {
                        Entity* e2 = LevelDataGetEntity(&CurLevel, k);
                        if (e2 == entity || e2 == player) {
                            continue;
                        }

                        if (Vector3Equals(child, World2Grid(e2->position))) {
                            hit = true;
                            break;
                        }
                    }

                    hmput(parents, child, curr);
                    if (!hit) {
                        assert(tail < 128);
                        queue[tail] = child;
                        ++tail;
                    }
                }
            }
        }

        ptrdiff_t idx = hmgeti(parents, end);
        assert(idx != -1);
        Vector3 next_move = end;
        Vector3 parent = parents[idx].value;
        while (!Vector3Equals(parent, start)) {
            next_move = parent;
            parent = hmget(parents, next_move);
        }

        // Reached player, signal level failed.
        // TOOD: Bug here. If player and hunter try to move to same square, player will not because of the way we
        // interpolate its position during movement. Need to figure this out in a way that doesn't mess with the
        // animation.
        if (Vector3Equals(next_move, end)) {
            CurLevel.failed = true;
            return;
        }

        next_move = GridToWorld(next_move);
        entity->position = next_move;
    }

    // TODO: Maybe improve this. Should think about centrallizing collision response since I want to respond differently
    // depending on what entity types are colliding. Its a bit hard otherwise to deal with this logic since I have to
    // spread the collision response logic across multiple update functions. Having the logic be spread in update
    // functions can be convenient though.
    if (Vector3Equals(World2Grid(entity->position), World2Grid(player->position))) {
        CurLevel.failed = true;
    }
}

static void EntityUpdate(Entity* entity, float delta_time) {
    if (EntityTestFlag(entity, EF_TRANSPARENT_ON_COLLISION)) {
        EntityTransparentOnCollision(entity, delta_time);
    }
    if (EntityTestFlag(entity, EF_LEVEL_SUCCESS_ON_COLLISION)) {
        EntityLevelSuccessOnCollision(entity, delta_time);
    }
    if (EntityTestFlag(entity, EF_PLAYER_CONTROLLED)) {
        EntityPlayerControlled(entity, delta_time);
    }
    if (EntityTestFlag(entity, EF_HUNT_PLAYER)) {
        EntityHuntPlayer(entity, delta_time);
    }
}

/////////////////////////////////////////////////////////////////////////////////////
static Color Colors[7] = {GRAY, RED, BLUE, GREEN, YELLOW, PINK, ORANGE};
static Color FaceColors[6] = {RED, BLUE, GREEN, YELLOW, PINK, ORANGE};

static Color GetTopFaceColor(Mesh* mesh, Quaternion rotation) {
    Vector3 up = {0.0f, 1.0f, 0.0f};
    for (uint32_t i = 0; i < (sizeof(FaceColors) / sizeof(FaceColors[0])); ++i) {
        uint32_t offset = i * 4 * 3;
        Vector3 normal = {mesh->normals[offset + 0], mesh->normals[offset + 1], mesh->normals[offset + 2]};
        normal = Vector3RotateByQuaternion(normal, rotation);
        float angle = Vector3Angle(normal, up);
        if (fabs(angle) <= 1e-5) {
            return FaceColors[i];
        }
    }

    assert(0);
}

static void PlayerInit(Entity* player, Vector3 position) {
    player->position = position;
    player->orientation = QuaternionIdentity();
    player->model = GenCubeModel(RED);
    EntitySetFlag(player, EF_PLAYER_CONTROLLED);

    Texture texture = GenCubeTexture2(FaceColors);

    // Remap the texture coordinates for all of the faces of the cube such that each face has a different color.
    player->model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;
    Mesh* mesh = &player->model.meshes[0];
    for (uint32_t i = 0; i < 6; ++i) {
        uint32_t tidx = i * 2 * 4;
        const float face_width = 128.0f;
        float min = (i * face_width);
        float max = min + face_width;
        float min_u = min / texture.width;
        float max_u = max / texture.width;
        mesh->texcoords[tidx + 0] = min_u;
        mesh->texcoords[tidx + 1] = 0.0f;
        mesh->texcoords[tidx + 2] = max_u;
        mesh->texcoords[tidx + 3] = 0.0f;
        mesh->texcoords[tidx + 4] = max_u;
        mesh->texcoords[tidx + 5] = 1.0f;
        mesh->texcoords[tidx + 6] = min_u;
        mesh->texcoords[tidx + 7] = 1.0f;
    }
    // TODO: This feels hacky, but it works. Just forget the mesh buffer ids and reupload.
    mesh->vaoId = 0;
    mesh->vboId = NULL;
    UploadMesh(mesh, false);
}

static void EntityPlayerControlled(Entity* entity, float delta_time) {
    static float T = 0;
    static Quaternion StartOrientation;
    static Quaternion EndOrientation;
    static Vector3 StartPosition;
    static Vector3 EndPosition;

    switch (entity->player_state) {
        case PS_IDLE: {
            if (IsKeyPressed(KEY_UP)) {
                entity->player_state = PS_WALKING;
                T = 0;
                StartOrientation = entity->orientation;
                StartPosition = entity->position;
                EndOrientation =
                    QuaternionMultiply(QuaternionFromAxisAngle((Vector3){1, 0, 0}, -90 * DEG2RAD), entity->orientation);
                EndPosition = Vector3Add(entity->position, (Vector3){0, 0, -CubeSideLength});
            }
            if (IsKeyPressed(KEY_RIGHT)) {
                entity->player_state = PS_WALKING;
                T = 0;
                StartOrientation = entity->orientation;
                StartPosition = entity->position;
                EndOrientation =
                    QuaternionMultiply(QuaternionFromAxisAngle((Vector3){0, 0, 1}, -90 * DEG2RAD), entity->orientation);
                EndPosition = Vector3Add(entity->position, (Vector3){CubeSideLength, 0, 0});
            }
            if (IsKeyPressed(KEY_DOWN)) {
                entity->player_state = PS_WALKING;
                T = 0;
                StartOrientation = entity->orientation;
                StartPosition = entity->position;
                EndOrientation =
                    QuaternionMultiply(QuaternionFromAxisAngle((Vector3){1, 0, 0}, 90 * DEG2RAD), entity->orientation);
                EndPosition = Vector3Add(entity->position, (Vector3){0, 0, CubeSideLength});
            }
            if (IsKeyPressed(KEY_LEFT)) {
                entity->player_state = PS_WALKING;
                T = 0;
                StartOrientation = entity->orientation;
                StartPosition = entity->position;
                EndOrientation =
                    QuaternionMultiply(QuaternionFromAxisAngle((Vector3){0, 0, 1}, 90 * DEG2RAD), entity->orientation);
                EndPosition = Vector3Add(entity->position, (Vector3){-CubeSideLength, 0, 0});
            }

            break;
        }
        case PS_WALKING: {
            T += delta_time;
            float t = T / 0.15;
            if (t >= 1.0f) {
                entity->player_state = PS_IDLE;
                entity->orientation = EndOrientation;
                entity->position = EndPosition;
            } else {
                entity->orientation = QuaternionSlerp(StartOrientation, EndOrientation, t);
                entity->position = Vector3Lerp(StartPosition, EndPosition, t);
            }

            Color top_color = GetTopFaceColor(&entity->model.meshes[0], StartOrientation);

            Entity* goal = LevelDataGetGoal(&CurLevel);
            BoundingBox player_bb = GetBoundingBox(entity->position);

            for (uint32_t i = 0; i < LevelDataNumEntities(&CurLevel); ++i) {
                Entity* entity2 = LevelDataGetEntity(&CurLevel, i);
                if (entity2 == entity || entity2 == goal) {
                    continue;
                }

                Color e2_color = entity2->block_color;
                BoundingBox e2_bb = GetBoundingBox(entity2->position);
                if (CheckCollisionBoxes(player_bb, e2_bb) &&
                    (top_color.r != e2_color.r || top_color.g != e2_color.g || top_color.b != e2_color.b ||
                     top_color.a != e2_color.a) &&
                    !EntityTestFlag(entity2, EF_HUNT_PLAYER)) {
                    entity->player_state = PS_IDLE;
                    entity->position = StartPosition;
                    entity->orientation = StartOrientation;
                }
            }

            break;
        }
        default: {
            exit(0);
        }
    }

    if (false) {
        PrintMatrix(QuaternionToMatrix(entity->orientation));
    }
}

/////////////////////////////////////////////////////////////////////////////////////
// Serde.
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

/////////////////////////////////////////////////////////////////////////////////////
typedef enum EditorState {
    ES_IDLE = 0,
    ES_EDITING,
    ES_SAVING,
    ES_LOADING,
    ES_SELECT_FOCUS,

    ES_COUNT
} EditorState;

typedef struct Editor {
    EditorState state;
    // Index to entity we are focused on.
    uint32_t focus_idx;
    bool control_key_unpressed;
    char load_level_filename[256];
} Editor;

static Editor GameEditor = {0};
// Yuck -- figure out something better later.
static bool EditorEnabled = false;

static void EditorEnable() {
    EditorEnabled = true;
    GameEditor = (Editor){0};
}

static void EditorDisable() {
    EditorEnabled = false;
    GameEditor = (Editor){0};
}

static Entity* EditorGetFocus(Editor* editor) { return &CurLevel.entities[editor->focus_idx]; }

static Color EntityGetFocusColor(Color color) { return (Color){color.r, color.g, color.b, 255 / 2}; }

static void EditorUpdate(Editor* editor) {
    static uint32_t color_idx = 0;

    assert(EditorEnabled);

    // TODO: Figure out something better here. I can't control-e to toggle editor and control-s to save without lifting
    // off control key.
    if (!IsKeyDown(KEY_LEFT_CONTROL)) {
        editor->control_key_unpressed = true;
    }

    switch (editor->state) {
        case ES_IDLE: {
            if (IsKeyPressed(KEY_A)) {
                Entity* focus = CreateLevelDataEntity(&CurLevel);
                focus->model = GenCubeModel(GRAY);
                focus->model.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = EntityGetFocusColor(focus->block_color);
                focus->position = ORIGIN;
                editor->focus_idx = focus->idx;
                editor->state = ES_EDITING;
                color_idx = 0;
            } else if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyDown(KEY_LEFT_SHIFT) && IsKeyDown(KEY_S) &&
                       editor->control_key_unpressed) {
                // TOOD: Add text fade to screen.
                editor->control_key_unpressed = false;
                LevelSetStart(&CurLevel);
            } else if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyDown(KEY_S) && editor->control_key_unpressed) {
                editor->control_key_unpressed = false;
                editor->state = ES_SAVING;
            } else if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyDown(KEY_L) && editor->control_key_unpressed) {
                editor->control_key_unpressed = false;
                editor->state = ES_LOADING;
            } else if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyDown(KEY_F) && editor->control_key_unpressed) {
                editor->control_key_unpressed = false;
                editor->focus_idx = 0;
                editor->state = ES_SELECT_FOCUS;
            }
            break;
        }
        case ES_EDITING: {
            Entity* focus = EditorGetFocus(editor);
            if (IsKeyPressed(KEY_UP)) {
                focus->position.z -= CubeSideLength;
            } else if (IsKeyPressed(KEY_RIGHT)) {
                focus->position.x += CubeSideLength;
            } else if (IsKeyPressed(KEY_DOWN)) {
                focus->position.z += CubeSideLength;
            } else if (IsKeyPressed(KEY_LEFT)) {
                focus->position.x -= CubeSideLength;
            } else if (IsKeyPressed(KEY_PAGE_UP)) {
                focus->position.y += CubeSideLength;
            } else if (IsKeyPressed(KEY_PAGE_DOWN)) {
                focus->position.y -= CubeSideLength;
            } else if (IsKeyPressed(KEY_ENTER)) {
                focus->model.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = WHITE;
                focus = NULL;
                editor->state = ES_IDLE;
            } else if (IsKeyPressed(KEY_C)) {
                color_idx = (color_idx + 1) % (sizeof(Colors) / sizeof(Colors[0]));
                focus->block_color = Colors[color_idx];
                focus->model = GenCubeModel(focus->block_color);
                focus->model.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = EntityGetFocusColor(focus->block_color);
            }
            break;
        }
        case ES_SAVING: {
            if (IsKeyPressed(KEY_ENTER)) {
                SaveLevel(&CurLevel);
                editor->state = ES_IDLE;
            } else if (IsKeyPressed(KEY_BACKSPACE)) {
                uint32_t len = strlen(CurLevel.name);
                if (len > 0) {
                    CurLevel.name[len - 1] = '\0';
                }
            } else {
                int key = GetCharPressed();
                uint32_t name_len = strlen(CurLevel.name);
                if ((key >= 32) && (key <= 125) && (name_len < 255)) {  // 255 to allow for null terminator.
                    CurLevel.name[name_len] = (char)key;
                    CurLevel.name[name_len + 1] = '\0';
                }
            }
            break;
        }
        case ES_LOADING: {
            if (IsKeyPressed(KEY_ENTER)) {
                LoadLevel(editor->load_level_filename);
                editor->state = ES_IDLE;
            } else if (IsKeyPressed(KEY_BACKSPACE)) {
                uint32_t len = strlen(editor->load_level_filename);
                if (len > 0) {
                    editor->load_level_filename[len - 1] = '\0';
                }
            } else {
                int key = GetCharPressed();
                uint32_t name_len = strlen(editor->load_level_filename);
                if ((key >= 32) && (key <= 125) && (name_len < 255)) {  // 255 to allow for null terminator.
                    editor->load_level_filename[name_len] = (char)key;
                    editor->load_level_filename[name_len + 1] = '\0';
                }
            }
            break;
        }
        case ES_SELECT_FOCUS: {
            if (IsKeyPressed(KEY_ENTER)) {
                Entity* entity = EditorGetFocus(editor);
                editor->state = ES_EDITING;
                for (uint32_t i = 0; i < (sizeof(Colors) / sizeof(Colors[0])); ++i) {
                    Color c = Colors[i];
                    if (c.r == entity->block_color.r && c.g == entity->block_color.g && c.b == entity->block_color.b) {
                        color_idx = i;
                    }
                }
            } else {
                Entity* entity = EditorGetFocus(editor);
                entity->model.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = WHITE;

                int32_t dir = 0;
                if (IsKeyPressed(KEY_UP)) {
                    dir = 1;
                } else if (IsKeyPressed(KEY_DOWN)) {
                    dir = -1;
                }
                editor->focus_idx = (editor->focus_idx + dir) % LevelDataNumEntities(&CurLevel);

                entity = EditorGetFocus(editor);
                entity->model.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = EntityGetFocusColor(entity->block_color);
            }
            break;
        }
        default: {
            exit(EXIT_FAILURE);
        }
    }
}

static void EditorDraw(Editor* editor, float delta_time) {
    static bool show_cursor = true;
    static float show_cursor_cooldown = 0.0f;

    if (EditorEnabled) {
        show_cursor_cooldown += delta_time;
        if (show_cursor_cooldown >= 0.650f) {
            show_cursor = !show_cursor;
            show_cursor_cooldown = 0.0f;
        }

        // Not in Mode3D. All coords are screen space.
        DrawFPS(10, 10);
        DrawText("Editor Enabled", 100, 10, 20, BLACK);

        switch (editor->state) {
            case ES_SAVING: {
                char text[1024] = {0};
                sprintf(text, "Save level as: %s%c", CurLevel.name, show_cursor ? '|' : '\0');
                DrawText(text, 200, 40, 20, BLACK);
                break;
            }
            case ES_LOADING: {
                char text[1024] = {0};
                sprintf(text, "Load level: %s%c", editor->load_level_filename, show_cursor ? '|' : '\0');
                DrawText(text, 200, 40, 20, BLACK);
            }
            default: {
                break;
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////

static void GameInit() {
    InitWindow(ScreenWidth, ScreenHeight, "Game");

    SetTargetFPS(60);

    LoadLevel("level_0");

    GameCameraInit(&GameCamera);
}

static void GameUpdate(float delta_time) {
    static bool editor_just_toggled = false;
    if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyDown(KEY_E) && !editor_just_toggled) {
        editor_just_toggled = true;
        if (!EditorEnabled) {
            EditorEnable();
        } else {
            EditorDisable();
        }
    } else if (!(IsKeyDown(KEY_LEFT_CONTROL) && IsKeyDown(KEY_E))) {
        editor_just_toggled = false;
    }

    if (!EditorEnabled) {
        if (CurLevel.finished) {
            if (IsKeyPressed(KEY_ENTER)) {
                LoadLevel(CurLevel.name);
            }
        } else if (CurLevel.failed) {
            if (IsKeyPressed(KEY_ENTER)) {
                LoadLevel(CurLevel.name);
            }
        } else {
            for (uint32_t i = 0; i < LevelDataNumEntities(&CurLevel); ++i) {
                Entity* entity = LevelDataGetEntity(&CurLevel, i);
                EntityUpdate(entity, delta_time);
            }
        }

    } else {
        EditorUpdate(&GameEditor);
    }

    // Ensure we do this after all above updates so that the transforms reflect updated entity info.
    for (uint32_t i = 0; i < LevelDataNumEntities(&CurLevel); ++i) {
        Entity* entity = LevelDataGetEntity(&CurLevel, i);
        EntityUpdateTransform(entity);
    }
}

static void GameDraw(float delta_time) {
    ClearBackground(RAYWHITE);

    BeginDrawing();

    BeginMode3D(GameCamera);

    Vector3 position = {0};
    for (uint32_t i = 0; i < LevelDataNumEntities(&CurLevel); ++i) {
        Entity* entity = LevelDataGetEntity(&CurLevel, i);
        DrawModel(entity->model, position, 1, WHITE);
    }

    EndMode3D();
    EditorDraw(&GameEditor, delta_time);

    if (CurLevel.finished) {
        char text[256] = {0};
        sprintf(text, "Level Completed! Press Enter to Replay");
        DrawText(text, 20 + 5, 20 + 5, 18, BLACK);
    } else if (CurLevel.failed) {
        char text[256] = {0};
        sprintf(text, "Level Failed! Press Enter to Replay");
        DrawText(text, 20 + 5, 20 + 5, 18, BLACK);
    }

    EndDrawing();
}

int main(void) {
    GameInit();

    while (!WindowShouldClose()) {
        float delta_time = GetFrameTime();
        GameUpdate(delta_time);
        GameDraw(delta_time);
    }

    CloseWindow();

    return 0;
}