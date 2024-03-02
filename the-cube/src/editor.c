#ifndef UNITY_BUILD
#include <stdbool.h>

#include "entity.c"
#include "level.c"
#include "raylib.h"

#endif

//////////////////////////////////////////////////////////////////////
// Forward decls
static void EnableGameUpdate();
static void DisableGameUpdate();
//////////////////////////////////////////////////////////////////////

typedef enum EditorState {
    ES_IDLE = 0,
    ES_EDITING,
    ES_SAVING,
    ES_LOADING,

    ES_COUNT
} EditorState;

#define EditorNumEntities 3

typedef struct Editor {
    EditorState state;
    // Index to entity we are focused on.
    uint32_t focus_idx;
    char load_level_filename[256];

    union {
        struct {
            Entity x_arrow;
            Entity y_arrow;
            Entity z_arrow;
        };

        Entity entities[EditorNumEntities];
    };
} Editor;

static Editor GameEditor = {0};
// Yuck -- figure out something better later.
static bool EditorEnabled = false;
#define IsEditorEnabled() (EditorEnabled)

static void EditorEnable() {
    EditorEnabled = true;
    GameEditor = (Editor){0};

    DisableGameUpdate();
}

static void EditorDisable() {
    EditorEnabled = false;
    GameEditor = (Editor){0};
    EnableGameUpdate();
}

static Entity *EditorGetFocus(Editor *editor) { return LevelDataGetEntity(GetCurLevel(), editor->focus_idx); }

static Color EntityGetFocusColor(Color color) { return (Color){color.r, color.g, color.b, 255 / 2}; }

static void EditorSetFocus(Editor *editor, Entity *focus) {
    focus->model.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = EntityGetFocusColor(focus->block_color);
    editor->focus_idx = focus->idx;
    editor->state = ES_EDITING;

    editor->x_arrow.model = LoadModelFromMesh(GenMeshCube(0.5, 6, 0.5));
    editor->x_arrow.model.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = BLUE;
    EntityTransformMesh(&editor->x_arrow,
                        MatrixMultiply(MatrixTranslate(0, 3, 0), MatrixRotate((Vector3){0, 0, 1}, -90 * DEG2RAD)));

    editor->y_arrow.model = LoadModelFromMesh(GenMeshCube(0.5, 6, 0.5));
    editor->y_arrow.model.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = GREEN;
    EntityTransformMesh(&editor->y_arrow,
                        MatrixMultiply(MatrixTranslate(0, 3, 0), MatrixRotate((Vector3){0, 0, 1}, 0 * DEG2RAD)));

    editor->z_arrow.model = LoadModelFromMesh(GenMeshCube(0.5, 6, 0.5));
    editor->z_arrow.model.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = RED;
    EntityTransformMesh(&editor->z_arrow,
                        MatrixMultiply(MatrixTranslate(0, 3, 0), MatrixRotate((Vector3){1, 0, 0}, 90 * DEG2RAD)));
}

static void EditorUpdate(Editor *editor, float delta_time) {
    static uint32_t color_idx = 0;
    assert(EditorEnabled);

    switch (editor->state) {
        case ES_IDLE: {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                Vector2 mouse_position = GetMousePosition();
                Ray mouse_click = GetMouseRay(mouse_position, GameCamera);

                LevelData *cur_level = GetCurLevel();
                RayCollisionLevelData collision = GetRayCollisionLevelData(mouse_click, cur_level);
                if (collision.collision.hit) {
                    Entity *entity = LevelDataGetEntity(cur_level, collision.entity_idx);
                    EditorSetFocus(editor, entity);
                    color_idx = 0;
                }
            }

            if (IsKeyPressed(KEY_A)) {
                Entity *focus = CreateLevelDataEntity(GetCurLevel());
                focus->model = GenCubeModel(GRAY);
                focus->position = ORIGIN;
                EditorSetFocus(editor, focus);
                color_idx = 0;
            } else if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_SHIFT)) {
                if (IsKeyPressed(KEY_S)) {
                    editor->state = ES_SAVING;
                } else if (IsKeyPressed(KEY_L)) {
                    editor->state = ES_LOADING;
                }
            }
            break;
        }
        case ES_EDITING: {
            Entity *focus = EditorGetFocus(editor);
            if (IsKeyPressed(KEY_ENTER)) {
                focus->model.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = WHITE;
                focus = NULL;
                editor->state = ES_IDLE;
            } else {
                if (IsKeyPressed(KEY_BACKSPACE)) {
                    LevelData *level_data = GetCurLevel();
                    LevelDataDeleteEntity(level_data, editor->focus_idx);
                    editor->state = ES_IDLE;
                } else if (IsKeyPressed(KEY_UP)) {
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
                } else if (IsKeyPressed(KEY_C)) {
                    color_idx = (color_idx + 1) % (sizeof(Colors) / sizeof(Colors[0]));
                    focus->block_color = Colors[color_idx];
                    focus->model = GenCubeModel(focus->block_color);
                    focus->model.materials[0].maps[MATERIAL_MAP_DIFFUSE].color =
                        EntityGetFocusColor(focus->block_color);
                }
            }
            break;
        }
        case ES_SAVING: {
            if (IsKeyPressed(KEY_ENTER)) {
                SaveLevel(GetCurLevel());
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
        default: {
            exit(EXIT_FAILURE);
        }
    }

    // UpdateEntities.
    Entity *focus = EditorGetFocus(editor);
    for (uint32_t i = 0; i < 3; ++i) {
        Entity *entity = &editor->entities[i];
        entity->position = focus->position;
        entity->orientation = focus->orientation;
    }

    for (uint32_t i = 0; i < EditorNumEntities; ++i) {
        Entity *entity = &editor->entities[i];
        EntityUpdateTransform(entity);
    }
}

static void EditorDraw(Editor *editor, float delta_time) {
    static bool show_cursor = true;
    static float show_cursor_cooldown = 0.0f;

    show_cursor_cooldown += delta_time;
    if (show_cursor_cooldown >= 0.650f) {
        show_cursor = !show_cursor;
        show_cursor_cooldown = 0.0f;
    }

    // Not in Mode3D. All coords are screen space.
    DrawFPS(10, 10);
    DrawText("Editor Enabled", 100, 10, 20, BLACK);

    BeginMode3D(GameCamera);

    if (editor->state == ES_EDITING) {
        Vector3 zero = {0};
        for (uint32_t i = 0; i < EditorNumEntities; ++i) {
            Entity *entity = &editor->entities[i];
            DrawModel(entity->model, zero, 1, WHITE);
        }
    }

    EndMode3D();

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