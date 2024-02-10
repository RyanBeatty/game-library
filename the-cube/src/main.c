#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

const int ScreenWidth = 800;
const int ScreenHeight = 450;

static const float CubeSideLength = 2.0f;

static void PrintMatrix(Matrix m) {
    float16 mv = MatrixToFloatV(m);
    for (int i = 0; i < 16; ++i) {
        if (i % 4 == 0) {
            printf("\n");
        }
        printf("%f ", mv.v[i]);
    }
    printf("\n");
}

typedef struct Entity {
    uint32_t idx;
    Vector3 position;
    Quaternion orientation;

    Model model;
} Entity;

static void EntityUpdateTransform(Entity* entity) {
    // Remember matrices are applied left to right order, so the rotation is first.
    entity->model.transform =
        MatrixMultiply(QuaternionToMatrix(entity->orientation),
                       MatrixTranslate(entity->position.x, entity->position.y, entity->position.z));
}

struct {
    Color key;
    Texture value;
}* TextureCache = NULL;

static Texture GenCubeTexture(Color color) {
    Image image = GenImageColor(128, 128, color);
    // Horizontal lines.
    const int thickness = 5;
    ImageDrawRectangle(&image, 0, 0, 128, thickness, BLACK);
    ImageDrawRectangle(&image, 0, 128 - thickness, 128, thickness, BLACK);
    // Vertical lines.
    ImageDrawRectangle(&image, 0, 0, thickness, 128, BLACK);
    ImageDrawRectangle(&image, 128 - thickness, 0, thickness, 128, BLACK);

    Texture texture = LoadTextureFromImage(image);
    UnloadImage(image);
    return texture;
}

static Texture GenCubeTexture2(Color colors[6]) {
    const int width = 128;
    const int height = 128;
    const int edge_thickness = 5;
    const int num_colors = 6;
    Image image = GenImageColor(width * num_colors, height, BLACK);
    for (uint32_t i = 0; i < num_colors; ++i) {
        // Fill with color for face.
        ImageDrawRectangle(&image, i * width, 0, width, height, colors[i]);

        // Horizontal lines.
        ImageDrawRectangle(&image, 0, 0, width, edge_thickness, BLACK);
        ImageDrawRectangle(&image, 0, height - edge_thickness, width, edge_thickness, BLACK);
        // Vertical lines.
        ImageDrawRectangle(&image, 0, 0, edge_thickness, height, BLACK);
        ImageDrawRectangle(&image, width - edge_thickness, 0, edge_thickness, height, BLACK);
    }

    Texture texture = LoadTextureFromImage(image);
    UnloadImage(image);
    return texture;
}

static Texture GetTextureForColor(Color color) {
    ptrdiff_t idx = hmgeti(TextureCache, color);
    if (idx == -1) {
        Texture texture = GenCubeTexture(color);
        hmput(TextureCache, color, texture);
        return texture;
    }
    return TextureCache[idx].value;
}

static Model GenCubeModel(Color color) {
    Model model = LoadModelFromMesh(GenMeshCube(CubeSideLength, CubeSideLength, CubeSideLength));
    model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = GetTextureForColor(color);
    return model;
}

static BoundingBox GetBoundingBox(Vector3 position) {
    // NOTE: Make the bounding box smaller than the cube for now so that we don't accidentally collide two boxes if they
    // move next to eachother.
    float radius = CubeSideLength / 4.0f;
    BoundingBox bb = {.min = Vector3Add(position, (Vector3){-radius, -radius, -radius}),
                      .max = Vector3Add(position, (Vector3){radius, radius, radius})};
    return bb;
}

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
// Level code

static char* GameLevelsPath = "resources/levels";

typedef struct LevelData {
    char name[256];
    bool finished;
    Vector3 start;
    Vector3 goal;
    // stbds array.
    Entity* entities;
} LevelData;

static LevelData CurLevel = {0};

static Entity* LevelDataCreateEntity(LevelData* level_data) {
    Entity* entity = arraddnptr(level_data->entities, 1);
    entity->idx = arrlen(level_data->entities) - 1;
    return entity;
}

static Entity* CreateLevelDataEntity(LevelData* level_data) {
    Entity* entity = LevelDataCreateEntity(level_data);
    return entity;
}

#define LevelDataGetStart (level_data)((level_data)->start)

static Entity* LevelDataGetPlayer(LevelData* level_data) { return &level_data->entities[0]; }

static Entity* LevelDataGetEntity(LevelData* level_data, uint32_t i) { return &level_data->entities[i]; }

static uint32_t LevelDataNumEntities(LevelData* level_data) { return arrlenu(level_data->entities); }

static void LevelSetStart(LevelData* level_data) {
    Entity* player = LevelDataGetPlayer(level_data);
    level_data->start = player->position;
}

static Entity* LevelDataGetGoal(LevelData* level_data) { return LevelDataGetEntity(level_data, 1); }

/////////////////////////////////////////////////////////////////////////////////////
typedef enum PlayerState {
    PS_IDLE = 0,
    PS_WALKING,

    PS_COUNT
} PlayerState;

static void PlayerInit(Entity* player, Vector3 position) {
    player->position = position;
    player->orientation = QuaternionIdentity();
    player->model = GenCubeModel(RED);

    Texture texture = GenCubeTexture2((Color[6]){RED, BLUE, GREEN, YELLOW, PINK, ORANGE});

    // Remap the texture coordinates for all of the faces of the cube such that each face has a different color.
    // TODO: Seems like the edges aren't quite working for all of the faces.
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

static void PlayerUpdate(Entity* entity, float delta_time) {
    static PlayerState State = PS_IDLE;
    static float T = 0;
    static Quaternion StartOrientation;
    static Quaternion EndOrientation;
    static Vector3 StartPosition;
    static Vector3 EndPosition;

    switch (State) {
        case PS_IDLE: {
            if (IsKeyPressed(KEY_UP)) {
                State = PS_WALKING;
                T = 0;
                StartOrientation = entity->orientation;
                StartPosition = entity->position;
                EndOrientation =
                    QuaternionMultiply(QuaternionFromAxisAngle((Vector3){1, 0, 0}, -90 * DEG2RAD), entity->orientation);
                EndPosition = Vector3Add(entity->position, (Vector3){0, 0, -CubeSideLength});
            }
            if (IsKeyPressed(KEY_RIGHT)) {
                State = PS_WALKING;
                T = 0;
                StartOrientation = entity->orientation;
                StartPosition = entity->position;
                EndOrientation =
                    QuaternionMultiply(QuaternionFromAxisAngle((Vector3){0, 0, 1}, -90 * DEG2RAD), entity->orientation);
                EndPosition = Vector3Add(entity->position, (Vector3){CubeSideLength, 0, 0});
            }
            if (IsKeyPressed(KEY_DOWN)) {
                State = PS_WALKING;
                T = 0;
                StartOrientation = entity->orientation;
                StartPosition = entity->position;
                EndOrientation =
                    QuaternionMultiply(QuaternionFromAxisAngle((Vector3){1, 0, 0}, 90 * DEG2RAD), entity->orientation);
                EndPosition = Vector3Add(entity->position, (Vector3){0, 0, CubeSideLength});
            }
            if (IsKeyPressed(KEY_LEFT)) {
                State = PS_WALKING;
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
            // Check for collision with goal.
            // TODO: Maybe rethink this since this causes problems depending on the size of the bounding box. for
            // example, if we make the bounding box the same size as cube, then we will accidentally collide if we move
            // to a square next to goal. Maybe instead we should convert entity positions to a "grid space" where each
            // grid spot has a unique id and use that to check collisions.
            Entity* goal = LevelDataGetGoal(&CurLevel);
            BoundingBox player_bb = GetBoundingBox(EndPosition);
            BoundingBox goal_bb = GetBoundingBox(goal->position);
            if (CheckCollisionBoxes(player_bb, goal_bb)) {
                CurLevel.finished = true;
                State = PS_IDLE;
                break;
            }

            for (uint32_t i = 0; i < LevelDataNumEntities(&CurLevel); ++i) {
                Entity* entity2 = LevelDataGetEntity(&CurLevel, i);
                if (entity2 == entity || entity2 == goal) {
                    continue;
                }

                BoundingBox e2_bb = GetBoundingBox(entity2->position);
                if (CheckCollisionBoxes(player_bb, e2_bb)) {
                    State = PS_IDLE;
                }
            }

            // We hit something, stop walking.
            if (State != PS_WALKING) {
                break;
            }

            T += delta_time;
            float t = T / 0.2;
            if (t >= 1.0f) {
                State = PS_IDLE;
                entity->orientation = EndOrientation;
                entity->position = EndPosition;
            } else {
                entity->orientation = QuaternionSlerp(StartOrientation, EndOrientation, t);
                entity->position = Vector3Lerp(StartPosition, EndPosition, t);
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
        // TODO: Probably temp until we figure out mesh storage.
        pos = PackColor(pos, entity->model.materials[0].maps[MATERIAL_MAP_DIFFUSE].color);

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

    // Unpack entities.
    while (line != NULL) {
        // Start buffer at the beginning of line, this is just so we have line around in case debuggin is needed.
        char* buffer = line;

        Entity* entity = CreateLevelDataEntity(level_data);

        buffer = UnpackU32(buffer, &entity->idx);
        buffer = UnpackVector3(buffer, &entity->position);
        buffer = UnpackQuaternion(buffer, &entity->orientation);

        Color color;
        buffer = UnpackColor(buffer, &color);
        entity->model = GenCubeModel(color);

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

static void EditorUpdate(Editor* editor) {
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
                focus->model.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = BLACK;
                focus->position = (Vector3){CubeSideLength / 2, CubeSideLength / 2, CubeSideLength / 2};
                editor->focus_idx = focus->idx;
                editor->state = ES_EDITING;
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
                focus->model.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = GRAY;
                focus = NULL;
                editor->state = ES_IDLE;
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
                editor->state = ES_EDITING;
            } else {
                Entity* entity = EditorGetFocus(editor);
                entity->model.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = GRAY;

                int32_t dir = 0;
                if (IsKeyPressed(KEY_UP)) {
                    dir = 1;
                } else if (IsKeyPressed(KEY_DOWN)) {
                    dir = -1;
                }
                editor->focus_idx = (editor->focus_idx + dir) % LevelDataNumEntities(&CurLevel);

                entity = EditorGetFocus(editor);
                entity->model.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = BLACK;
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
        Entity* player = LevelDataGetPlayer(&CurLevel);
        PlayerUpdate(player, delta_time);
        // Detect hit goal and then reset the level.
        if (CurLevel.finished) {
            LoadLevel(CurLevel.name);
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