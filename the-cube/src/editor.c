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

typedef struct Focus {
    uint32_t *entity_idxs;  // stb_arr.
} Focus;

static void FocusInit(Focus *focus) { focus->entity_idxs = NULL; }

static void FocusDeInit(Focus *focus) {
    LevelData *level_data = GetCurLevel();
    for (uint32_t i = 0; i < arrlenu(focus->entity_idxs); ++i) {
        Entity *entity = LevelDataGetEntity(level_data, focus->entity_idxs[i]);
        entity->model.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = WHITE;
    }
    focus->entity_idxs = NULL;
}

static void FocusAdd(Focus *focus, uint32_t idx) {
    // Insert in sorted order so we can delete okay.
    for (uint32_t i = 0; i < arrlenu(focus->entity_idxs); ++i) {
        if (idx < focus->entity_idxs[i]) {
            arrins(focus->entity_idxs, i, idx);
        }
    }
    arrpush(focus->entity_idxs, idx);
}

static void FocusDelete(Focus *focus) {
    LevelData *level_data = GetCurLevel();
    // Delete in reverse order so that our indexes are stable.
    for (int i = arrlenu(focus->entity_idxs) - 1; i >= 0; --i) {
        LevelDataDeleteEntity(level_data, focus->entity_idxs[i]);
    }
    focus->entity_idxs = NULL;
}

static void FocusMove(Focus *focus, Vector3 direction) {
    LevelData *level_data = GetCurLevel();
    for (uint32_t i = 0; i < arrlenu(focus->entity_idxs); ++i) {
        Entity *entity = LevelDataGetEntity(level_data, focus->entity_idxs[i]);
        entity->position = Vector3Add(entity->position, direction);
    }
}

static Vector3 FocusGetPosition(Focus *focus) {
    if (arrlenu(focus->entity_idxs) == 0) {
        return Vector3Zero();
    }
    return LevelDataGetEntity(GetCurLevel(), focus->entity_idxs[0])->position;
}

static Quaternion FocusGetOrientation(Focus *focus) {
    if (arrlenu(focus->entity_idxs) == 0) {
        return QuaternionIdentity();
    }
    return LevelDataGetEntity(GetCurLevel(), focus->entity_idxs[0])->orientation;
}

static Color EntityGetFocusColor(Color color) { return (Color){color.r, color.g, color.b, 255 / 2}; }

static void FocusChangeColor(Focus *focus, uint32_t color_idx) {
    LevelData *level_data = GetCurLevel();
    for (uint32_t i = 0; i < arrlenu(focus->entity_idxs); ++i) {
        Entity *entity = LevelDataGetEntity(level_data, i);
        entity->block_color = Colors[color_idx];
        entity->model = GenCubeModel(entity->block_color);
        entity->model.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = EntityGetFocusColor(entity->block_color);
    }
}

typedef struct Editor {
    EditorState state;
    // Index to entity we are focused on.
    Focus focus;
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

static Focus *EditorGetFocus(Editor *editor) { return &editor->focus; }

static void FocusFocus(Editor *editor, Entity *focus) {
    focus->model.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = EntityGetFocusColor(focus->block_color);
    FocusInit(&editor->focus);
    FocusAdd(&editor->focus, focus->idx);
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
                    FocusFocus(editor, entity);
                    color_idx = 0;
                }
            }

            if (IsKeyPressed(KEY_A)) {
                Entity *focus = CreateLevelDataEntity(GetCurLevel());
                focus->model = GenCubeModel(GRAY);
                focus->position = ORIGIN;
                FocusFocus(editor, focus);
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
            Focus *focus = EditorGetFocus(editor);
            if (IsKeyPressed(KEY_ENTER)) {
                FocusDeInit(focus);
                editor->state = ES_IDLE;
            } else {
                if (IsKeyPressed(KEY_BACKSPACE)) {
                    FocusDelete(focus);
                    editor->state = ES_IDLE;
                } else if (IsKeyPressed(KEY_C)) {
                    color_idx = (color_idx + 1) % (sizeof(Colors) / sizeof(Colors[0]));
                    FocusChangeColor(focus, color_idx);
                } else {
                    Vector3 direction = {0};
                    if (IsKeyPressed(KEY_UP)) {
                        direction.z -= CubeSideLength;
                    } else if (IsKeyPressed(KEY_RIGHT)) {
                        direction.x += CubeSideLength;
                    } else if (IsKeyPressed(KEY_DOWN)) {
                        direction.z += CubeSideLength;
                    } else if (IsKeyPressed(KEY_LEFT)) {
                        direction.x -= CubeSideLength;
                    } else if (IsKeyPressed(KEY_PAGE_UP)) {
                        direction.y += CubeSideLength;
                    } else if (IsKeyPressed(KEY_PAGE_DOWN)) {
                        direction.y -= CubeSideLength;
                    }

                    FocusMove(EditorGetFocus(editor), direction);
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
    Focus *focus = EditorGetFocus(editor);
    Vector3 focus_position = FocusGetPosition(focus);
    Quaternion focus_orientation = FocusGetOrientation(focus);
    for (uint32_t i = 0; i < 3; ++i) {
        Entity *entity = &editor->entities[i];
        entity->position = focus_position;
        entity->orientation = focus_orientation;
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