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
    ES_SELECT_FOCUS,

    ES_COUNT
} EditorState;

typedef struct Editor {
    EditorState state;
    // Index to entity we are focused on.
    uint32_t focus_idx;
    char load_level_filename[256];
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

static Entity* EditorGetFocus(Editor* editor) { return LevelDataGetEntity(GetCurLevel(), editor->focus_idx); }

static Color EntityGetFocusColor(Color color) { return (Color){color.r, color.g, color.b, 255 / 2}; }

static void EditorUpdate(Editor* editor) {
    static uint32_t color_idx = 0;
    assert(EditorEnabled);

    switch (editor->state) {
        case ES_IDLE: {
            if (IsKeyPressed(KEY_A)) {
                Entity* focus = CreateLevelDataEntity(GetCurLevel());
                focus->model = GenCubeModel(GRAY);
                focus->model.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = EntityGetFocusColor(focus->block_color);
                focus->position = ORIGIN;
                editor->focus_idx = focus->idx;
                editor->state = ES_EDITING;
                color_idx = 0;
            } else if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_SHIFT)) {
                if (IsKeyDown(KEY_LEFT_SHIFT)) {
                    if (IsKeyPressed(KEY_S)) {
                        LevelSetStart(GetCurLevel());
                    }
                } else if (IsKeyPressed(KEY_S)) {
                    editor->state = ES_SAVING;
                } else if (IsKeyPressed(KEY_L)) {
                    editor->state = ES_LOADING;
                } else if (IsKeyPressed(KEY_F)) {
                    editor->focus_idx = 0;
                    editor->state = ES_SELECT_FOCUS;
                }
            }
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
                editor->focus_idx = (editor->focus_idx + dir) % LevelDataNumEntities(GetCurLevel());

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