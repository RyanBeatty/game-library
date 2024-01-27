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

Entity* GameEntities = NULL;

static Entity* CreateEntity() {
    Entity entity = {0};
    entity.idx = arrlen(GameEntities);
    arrpush(GameEntities, entity);
    return &arrlast(GameEntities);
    // // Entity* entity = arraddnptr(GameEntities, 1);
    // entity->idx = arrlen(GameEntities) - 1;
    // return entity;
}

static void EntityUpdateTransform(Entity* entity) {
    // Remember matrices are applied left to right order, so the rotation is first.
    entity->model.transform =
        MatrixMultiply(QuaternionToMatrix(entity->orientation),
                       MatrixTranslate(entity->position.x, entity->position.y, entity->position.z));
}

/////////////////////////////////////////////////////////////////////////////////////
static void PlayerInit(Entity* player) {
    player->position = (Vector3){CubeSideLength / 2, CubeSideLength / 2, CubeSideLength / 2};
    player->orientation = QuaternionIdentity();
    player->model = LoadModelFromMesh(GenMeshCube(CubeSideLength, CubeSideLength, CubeSideLength));
    player->model.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = RED;
}

static void PlayerUpdate(Entity* entity, float delta_time) {
    static bool walking = false;
    static float T = 0;
    static Quaternion StartOrientation;
    static Quaternion EndOrientation;
    static Vector3 StartPosition;
    static Vector3 EndPosition;

    if (!walking) {
        if (IsKeyPressed(KEY_UP)) {
            walking = true;
            T = 0;
            StartOrientation = entity->orientation;
            StartPosition = entity->position;
            EndOrientation =
                QuaternionMultiply(QuaternionFromAxisAngle((Vector3){1, 0, 0}, -90 * DEG2RAD), entity->orientation);
            EndPosition = Vector3Add(entity->position, (Vector3){0, 0, -CubeSideLength});
        }
        if (IsKeyPressed(KEY_RIGHT)) {
            walking = true;
            T = 0;
            StartOrientation = entity->orientation;
            StartPosition = entity->position;
            EndOrientation =
                QuaternionMultiply(QuaternionFromAxisAngle((Vector3){0, 0, 1}, -90 * DEG2RAD), entity->orientation);
            EndPosition = Vector3Add(entity->position, (Vector3){CubeSideLength, 0, 0});
        }
        if (IsKeyPressed(KEY_DOWN)) {
            walking = true;
            T = 0;
            StartOrientation = entity->orientation;
            StartPosition = entity->position;
            EndOrientation =
                QuaternionMultiply(QuaternionFromAxisAngle((Vector3){1, 0, 0}, 90 * DEG2RAD), entity->orientation);
            EndPosition = Vector3Add(entity->position, (Vector3){0, 0, CubeSideLength});
        }
        if (IsKeyPressed(KEY_LEFT)) {
            walking = true;
            T = 0;
            StartOrientation = entity->orientation;
            StartPosition = entity->position;
            EndOrientation =
                QuaternionMultiply(QuaternionFromAxisAngle((Vector3){0, 0, 1}, 90 * DEG2RAD), entity->orientation);
            EndPosition = Vector3Add(entity->position, (Vector3){-CubeSideLength, 0, 0});
        }
    }

    if (walking) {
        T += delta_time;
        float t = T / 0.2;
        if (t >= 1.0f) {
            walking = false;
            entity->orientation = EndOrientation;
            entity->position = EndPosition;
        } else {
            entity->orientation = QuaternionSlerp(StartOrientation, EndOrientation, t);
            entity->position = Vector3Lerp(StartPosition, EndPosition, t);
        }
    }

    if (false) {
        PrintMatrix(QuaternionToMatrix(entity->orientation));
    }
}

/////////////////////////////////////////////////////////////////////////////////////
static Camera3D GameCamera;

void GameCameraInit(Camera3D* camera) {
    camera->position = (Vector3){0.0f, 10.0f, 20.0f};
    camera->target = (Vector3){0.0f, 0.0f, 0.0f};
    camera->up = (Vector3){0.0f, 1.0f, 0.0f};
    camera->fovy = 45.0f;
    camera->projection = CAMERA_PERSPECTIVE;
}

/////////////////////////////////////////////////////////////////////////////////////

// TODO: Maybe remove this. Was used to bootstrap level data for saving and loading.
// #define GameGridNumRows (10)
// #define GameGridNumCols (10)
// #define GameGridNumSpaces ((GameGridNumRows) * (GameGridNumCols))

// static void RegisterBackgroundEntities() {
//     float length = CubeSideLength;
//     float half_length = length / 2.0f;
//     for (int i = 0, r = 0; r < GameGridNumRows; ++r) {
//         for (int c = 0; c < GameGridNumCols; ++c, ++i) {
//             Entity* entity = CreateEntity();
//             entity->model = LoadModelFromMesh(GenMeshCube(CubeSideLength, CubeSideLength, CubeSideLength));
//             entity->position = (Vector3){(r * length) + half_length, -half_length, (c * length) + half_length};
//             // Hack shift position so that camera get capture full scene.
//             entity->position = Vector3Add(entity->position, (Vector3){-10.0f, 0, -10.0f});
//             entity->model.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = GRAY;
//         }
//     }
// }
/////////////////////////////////////////////////////////////////////////////////////
// Level code

static char* GameLevelsPath = "resources/levels";

typedef struct LevelData {
    char name[256];
    // stb_arr - pointers to entities managed by this level.
    Entity** entities;
} LevelData;

static LevelData CurLevel = {0};

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

static void SaveLevel(LevelData* level_data) {
    char* data = NULL;

    // Note skip the first Entity, its always the Player!
    for (uint32_t i = 1; i < arrlen(level_data->entities); ++i) {
        char buffer[1024] = {0};
        char* pos = buffer;
        Entity* entity = level_data->entities[i];
        int ii = sprintf(pos, "%u", entity->idx);
        pos = &buffer[ii];
        pos = PackDelim(pos);
        pos = PackVector3(pos, entity->position);
        pos = PackDelim(pos);
        pos = PackQuaternion(pos, entity->orientation);
        pos = PackDelim(pos);
        // TODO: Probably temp until we figure out mesh storage.
        pos = PackColor(pos, entity->model.materials[0].maps[MATERIAL_MAP_DIFFUSE].color);

        uint32_t len = strlen(buffer);
        for (uint32_t j = 0; j < len; ++j) {
            arrpush(data, buffer[j]);
        }
        arrpush(data, '\n');
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

static void LoadLevel(const char* name) {
    char filepath[1024];
    sprintf(filepath, "%s/%s", GameLevelsPath, name);
    char* data = LoadFileText(filepath);

    CurLevel = (LevelData){0};
    LevelData* level_data = &CurLevel;  // Do this so we don't lose the stbarry entity info.
    strcpy(level_data->name, name);

    int count = 0;
    char* line = strtok(data, "\n");
    while (line != NULL) {
        if (count == 7) {
            breakpoint();
        }
        // Start buffer at the beginning of line, this is just so we have line around in case debuggin is needed.
        char* buffer = line;

        Entity* entity = CreateEntity();

        Mesh mesh = GenMeshCube(CubeSideLength, CubeSideLength, CubeSideLength);
        Model model = LoadModelFromMesh(mesh);
        entity->model = model;
        entity->model.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = GRAY;

        buffer = UnpackU32(buffer, &entity->idx);
        buffer = UnpackVector3(buffer, &entity->position);
        buffer = UnpackQuaternion(buffer, &entity->orientation);

        // buffer = UnpackColor(buffer, &entity->model.materials[0].maps[MATERIAL_MAP_DIFFUSE].color);

        arrpush(level_data->entities, entity);
        ++count;
        line = strtok(NULL, "\n");
        break;
    }

    breakpoint();
}

/////////////////////////////////////////////////////////////////////////////////////
typedef enum EditorState {
    ES_IDLE = 0,
    ES_EDITING,
    ES_SAVING,

    ES_COUNT
} EditorState;

typedef struct Editor {
    EditorState state;
    Entity* focus;
    bool game_just_saved;
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

static void EditorUpdate(Editor* editor) {
    assert(EditorEnabled);

    if (!(IsKeyDown(KEY_LEFT_CONTROL) && IsKeyDown(KEY_S))) {
        editor->game_just_saved = false;
    }

    switch (editor->state) {
        case ES_IDLE: {
            if (IsKeyPressed(KEY_A)) {
                Entity* focus = CreateEntity();
                focus->model = LoadModelFromMesh(GenMeshCube(CubeSideLength, CubeSideLength, CubeSideLength));
                focus->position = (Vector3){CubeSideLength / 2, CubeSideLength / 2, CubeSideLength / 2};
                focus->model.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = BLACK;
                editor->focus = focus;
                editor->state = ES_EDITING;
            }
            if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyDown(KEY_S) && !editor->game_just_saved) {
                breakpoint();
                editor->game_just_saved = true;
                editor->state = ES_SAVING;
            }
            break;
        }
        case ES_EDITING: {
            if (IsKeyPressed(KEY_UP)) {
                editor->focus->position.z -= CubeSideLength;
            } else if (IsKeyPressed(KEY_RIGHT)) {
                editor->focus->position.x += CubeSideLength;
            } else if (IsKeyPressed(KEY_DOWN)) {
                editor->focus->position.z += CubeSideLength;
            } else if (IsKeyPressed(KEY_LEFT)) {
                editor->focus->position.x -= CubeSideLength;
            } else if (IsKeyPressed(KEY_PAGE_UP)) {
                editor->focus->position.y += CubeSideLength;
            } else if (IsKeyPressed(KEY_PAGE_DOWN)) {
                editor->focus->position.y -= CubeSideLength;
            } else if (IsKeyPressed(KEY_ENTER)) {
                editor->focus = NULL;
                editor->state = ES_IDLE;
            }
            break;
        }
        case ES_SAVING: {
            if (IsKeyPressed(KEY_ENTER)) {
                SaveLevel(&CurLevel);
                editor->state = ES_IDLE;
            }
            break;
        }
        default: {
            exit(EXIT_FAILURE);
        }
    }
}

static void EditorDraw(Editor* editor) {
    if (EditorEnabled) {
        // Not in Mode3D. All coords are screen space.
        DrawFPS(10, 10);
        DrawText("Editor Enabled", 100, 10, 20, BLACK);

        switch (editor->state) {
            case ES_SAVING: {
                char text[1024] = {0};
                sprintf(text, "Save level as: %s", CurLevel.name);
                DrawText(text, 200, 40, 20, BLACK);
                break;
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

    Entity* player = CreateEntity();
    PlayerInit(player);

    LoadLevel("level_0.csv");
    breakpoint();

    GameCameraInit(&GameCamera);
}

static void GameUpdate(float delta_time) {
    if (IsKeyPressed(KEY_E)) {
        if (!EditorEnabled) {
            EditorEnable();
        } else {
            EditorDisable();
        }
    }

    if (!EditorEnabled) {
        Entity* player = &GameEntities[0];
        PlayerUpdate(player, delta_time);
    } else {
        EditorUpdate(&GameEditor);
    }

    // Ensure we do this after all above updates so that the transforms reflect updated entity info.
    for (uint32_t i = 0; i < arrlen(GameEntities); ++i) {
        EntityUpdateTransform(&GameEntities[i]);
    }
}

static void GameDraw() {
    ClearBackground(RAYWHITE);

    BeginDrawing();
    BeginMode3D(GameCamera);

    Vector3 position = {0};
    for (uint32_t i = 0; i < arrlen(GameEntities); ++i) {
        DrawModel(GameEntities[i].model, position, 1, WHITE);
    }

    EndMode3D();
    EditorDraw(&GameEditor);

    EndDrawing();
}

int main(void) {
    GameInit();

    while (!WindowShouldClose()) {
        float delta_time = GetFrameTime();
        GameUpdate(delta_time);
        GameDraw();
    }

    CloseWindow();

    return 0;
}