#ifndef UNITY_BUILD
#include <errno.h>
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

#define EditorNumEntities 3

static Color EntityGetFocusColor(Color color) { return (Color){color.r, color.g, color.b, 255 / 2}; }

typedef struct Focus {
    uint32_t *entity_idxs;  // stb_arr.
} Focus;

static void FocusInit(Focus *focus) { focus->entity_idxs = NULL; }

static bool FocusOn(Focus *focus) { return arrlenu(focus->entity_idxs) != 0; }

static void FocusDeInit(Focus *focus) {
    LevelData *level_data = GetCurLevel();
    for (uint32_t i = 0; i < arrlenu(focus->entity_idxs); ++i) {
        Entity *entity = LevelDataGetEntity(level_data, focus->entity_idxs[i]);
        entity->model.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = WHITE;
    }
    focus->entity_idxs = NULL;
}

static void FocusAdd(Focus *focus, uint32_t idx) {
    LevelData *level_data = GetCurLevel();
    Entity *entity = LevelDataGetEntity(level_data, idx);
    entity->model.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = EntityGetFocusColor(entity->block_color);

    // Insert in sorted order so we can delete okay.
    uint32_t i = 0;
    for (; i < arrlenu(focus->entity_idxs); ++i) {
        // Don't allow duplicates.
        if (idx == focus->entity_idxs[i]) {
            return;
        }
        if (idx < focus->entity_idxs[i]) {
            break;
        }
    }
    arrins(focus->entity_idxs, i, idx);
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

static void FocusChangeColor(Focus *focus, uint32_t color_idx) {
    LevelData *level_data = GetCurLevel();
    for (uint32_t i = 0; i < arrlenu(focus->entity_idxs); ++i) {
        Entity *entity = LevelDataGetEntity(level_data, focus->entity_idxs[i]);
        entity->block_color = Colors[color_idx];
        entity->model = GenCubeModel(entity->block_color);
        entity->model.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = EntityGetFocusColor(entity->block_color);
    }
}

static void FocusSetFlag(Focus *focus, EntityFlag flag) {
    LevelData *level = GetCurLevel();
    for (uint32_t i = 0; i < arrlenu(focus->entity_idxs); ++i) {
        Entity *entity = LevelDataGetEntity(level, focus->entity_idxs[i]);
        EntitySetFlag(entity, flag);
    }
}

static void FocusUnsetFlag(Focus *focus, EntityFlag flag) {
    LevelData *level = GetCurLevel();
    for (uint32_t i = 0; i < arrlenu(focus->entity_idxs); ++i) {
        Entity *entity = LevelDataGetEntity(level, focus->entity_idxs[i]);
        EntityUnsetFlag(entity, flag);
    }
}

static bool FocusIsInFocus(Focus *focus, uint32_t entity_idx, uint32_t *idx) {
    for (uint32_t i = 0; i < arrlenu(focus->entity_idxs); ++i) {
        if (entity_idx == focus->entity_idxs[i]) {
            *idx = i;
            return true;
        }
    }
    *idx = 0;
    return false;
}

static void FocusRemove(Focus *focus, uint32_t idx) {
    LevelData *level = GetCurLevel();
    Entity *entity = LevelDataGetEntity(level, focus->entity_idxs[idx]);
    entity->model.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = WHITE;
    arrdel(focus->entity_idxs, idx);
}

typedef struct EditorState {
    // Index to entity we are focused on.
    Focus focus;
    char load_level_filename[256];

    bool saving;

    union {
        struct {
            Entity x_arrow;
            Entity y_arrow;
            Entity z_arrow;
        };

        Entity entities[EditorNumEntities];
    };
} EditorState;

static EditorState Editor = {0};
// Yuck -- figure out something better later.
static bool EditorEnabled = false;
#define IsEditorEnabled() (EditorEnabled)

static void EditorToggle() {
    if (EditorEnabled) {
        // Enable.
        EditorEnabled = false;
        Editor = (EditorState){0};
        EnableGameUpdate();
    } else {
        // Disable.
        EditorEnabled = true;
        Editor = (EditorState){0};

        DisableGameUpdate();
    }
}

static Focus *EditorGetFocus(EditorState *editor) { return &editor->focus; }

static Focus *EditorSetFocus(EditorState *editor, Entity *focus) {
    FocusInit(&editor->focus);
    FocusAdd(&editor->focus, focus->idx);

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
    return EditorGetFocus(editor);
}

static void EditorUpdateEx(EditorState *editor, float delta_time) {
    static uint32_t color_idx = 0;
    assert(EditorEnabled);

    UIText("Editor greeting", "Editor Enabled");

    Camera *camera = GetGameCamera();

    Focus *focus = EditorGetFocus(editor);
    assert(focus != NULL);
    if (!FocusOn(focus)) {
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
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
    } else {
        Vector3 focus_pos_world = FocusGetPosition(focus);
        Vector2 focus_pos_screen = GetWorldToScreen(focus_pos_world, *camera);

        // Draw arrows.
        Color colors[3] = {BLUE, GREEN, RED};
        Vector3 arrows[3] = {(Vector3){1, 0, 0}, (Vector3){0, 1, 0}, (Vector3){0, 0, 1}};
        for (uint32_t i = 0; i < 3; ++i) {
            // Scale and Translate arrows from model space to world space and then to screen space.
            // TODO: Should just use the model transform to do the translate and incorporate the rotation.
            Vector3 arrow_pos_world = Vector3Add(focus_pos_world, Vector3Scale(arrows[i], 3));
            Vector2 arrow_pos_screen = GetWorldToScreen(arrow_pos_world, *camera);
            DrawLineEx(focus_pos_screen, arrow_pos_screen, 10, colors[i]);
        }
    }

    // Level saving.
    {
        bool saving_toggled = IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_S);
        if (saving_toggled) {
            if (editor->saving) {
                editor->saving = false;
            } else {
                editor->saving = true;
            }
        }

        if (editor->saving) {
            UIRow("saving row") {
                UIText("saving", "Saving");
                UIEditBox("level edit", editor->load_level_filename);
            }
        }
    }

    // switch (editor->state) {
    //     case ES_IDLE: {
    //         if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
    //             Vector2 mouse_position = GetMousePosition();
    //             Ray mouse_click = GetMouseRay(mouse_position, GameCamera);

    //             LevelData *cur_level = GetCurLevel();
    //             RayCollisionLevelData collision = GetRayCollisionLevelData(mouse_click, cur_level);
    //             if (collision.collision.hit) {
    //                 Entity *entity = LevelDataGetEntity(cur_level, collision.entity_idx);
    //                 EditorSetFocus(editor, entity);
    //                 color_idx = 0;
    //             }
    //         }

    //         if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_SHIFT)) {
    //             if (IsKeyPressed(KEY_S)) {
    //                 editor->state = ES_SAVING;
    //             } else if (IsKeyPressed(KEY_L)) {
    //                 editor->state = ES_LOADING;
    //             } else if (IsKeyPressed(KEY_R)) {
    //                 LoadLevel(GetCurLevel()->name);
    //             } else if (IsKeyPressed(KEY_A)) {
    //                 LevelData *level = GetCurLevel();
    //                 Entity *entity = LevelDataGetEntity(level, 0);
    //                 Focus *focus = EditorSetFocus(editor, entity);
    //                 for (uint32_t i = 1; i < LevelDataNumEntities(level); ++i) {
    //                     entity = LevelDataGetEntity(level, i);
    //                     FocusAdd(focus, entity->idx);
    //                 }
    //                 editor->state = ES_EDITING;
    //             }
    //         } else if (IsKeyPressed(KEY_C)) {
    //             editor->state = ES_CAMERA;
    //         } else if (IsKeyPressed(KEY_A)) {
    //             Entity *focus = LevelDataCreateEntity(GetCurLevel());
    //             focus->block_color = GRAY;
    //             focus->model = GenCubeModel(GRAY);
    //             focus->position = ORIGIN;
    //             EntitySetFlag(focus, EF_WALL);
    //             EditorSetFocus(editor, focus);
    //             color_idx = 0;
    //         }
    //         break;
    //     }
    //     case ES_EDITING: {
    //         {
    //             if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    //                 // TODO: Remove duplication.
    //                 Vector2 mouse_position = GetMousePosition();
    //                 Ray mouse_click = GetMouseRay(mouse_position, GameCamera);

    //                 LevelData *cur_level = GetCurLevel();
    //                 RayCollisionLevelData collision = GetRayCollisionLevelData(mouse_click, cur_level);
    //                 if (collision.collision.hit) {
    //                     uint32_t idx = 0;
    //                     Focus *focus = EditorGetFocus(editor);
    //                     if (FocusIsInFocus(focus, collision.entity_idx, &idx)) {
    //                         breakpoint();
    //                         FocusRemove(focus, idx);
    //                     } else {
    //                         FocusAdd(focus, collision.entity_idx);
    //                     }
    //                 }
    //             }
    //         }

    //         Focus *focus = EditorGetFocus(editor);

    //         int key = GetKeyPressed();
    //         static UIWidget EditFlagsTextBox = {0};
    //         if (UITextBox(&EditFlagsTextBox, "EditFlags", key == KEY_F, 200, 40, key)) {
    //             char *endptr;
    //             long flag = strtol(EditFlagsTextBox.buffer, &endptr, 10);
    //             assert(errno != ERANGE);
    //             assert(*endptr == '\0');
    //             if (flag < EF_COUNT) {
    //                 if (flag >= 0) {
    //                     FocusSetFlag(focus, (EntityFlag)flag);
    //                 } else {
    //                     flag *= -1;
    //                     FocusUnsetFlag(focus, (EntityFlag)flag);
    //                 }
    //             } else {
    //                 TraceLog(LOG_INFO, "EditFlags Flag Value Too High: %d > %d", flag, EF_COUNT);
    //             }
    //         }

    //         if (key == KEY_ENTER) {
    //             FocusDeInit(focus);
    //             editor->state = ES_IDLE;
    //         } else {
    //             if (key == KEY_BACKSPACE) {
    //                 FocusDelete(focus);
    //                 editor->state = ES_IDLE;
    //             } else if (key == KEY_C) {
    //                 color_idx = (color_idx + 1) % (sizeof(Colors) / sizeof(Colors[0]));
    //                 FocusChangeColor(focus, color_idx);
    //             } else {
    //                 Vector3 direction = {0};
    //                 if (key == KEY_UP) {
    //                     direction.z -= CubeSideLength;
    //                 } else if (key == KEY_RIGHT) {
    //                     direction.x += CubeSideLength;
    //                 } else if (key == KEY_DOWN) {
    //                     direction.z += CubeSideLength;
    //                 } else if (key == KEY_LEFT) {
    //                     direction.x -= CubeSideLength;
    //                 } else if (key == KEY_PAGE_UP) {
    //                     direction.y += CubeSideLength;
    //                 } else if (key == KEY_PAGE_DOWN) {
    //                     direction.y -= CubeSideLength;
    //                 }

    //                 FocusMove(EditorGetFocus(editor), direction);
    //             }
    //         }
    //         break;
    //     }
    //     case ES_FOCUSING: {
    //         if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
    //             editor->state = ES_EDITING;
    //         } else {
    //             // TODO: Remove duplication.
    //             Vector2 mouse_position = GetMousePosition();
    //             Ray mouse_click = GetMouseRay(mouse_position, GameCamera);

    //             LevelData *cur_level = GetCurLevel();
    //             RayCollisionLevelData collision = GetRayCollisionLevelData(mouse_click, cur_level);
    //             if (collision.collision.hit) {
    //                 Focus *focus = EditorGetFocus(editor);
    //                 FocusAdd(focus, collision.entity_idx);
    //             }
    //         }
    //         break;
    //     }
    //     case ES_SAVING: {
    //         if (IsKeyPressed(KEY_ENTER)) {
    //             SaveLevel(GetCurLevel());
    //             editor->state = ES_IDLE;
    //         } else if (IsKeyPressed(KEY_BACKSPACE)) {
    //             uint32_t len = strlen(CurLevel.name);
    //             if (len > 0) {
    //                 CurLevel.name[len - 1] = '\0';
    //             }
    //         } else {
    //             int key = GetCharPressed();
    //             uint32_t name_len = strlen(CurLevel.name);
    //             if ((key >= 32) && (key <= 125) && (name_len < 255)) {  // 255 to allow for null terminator.
    //                 CurLevel.name[name_len] = (char)key;
    //                 CurLevel.name[name_len + 1] = '\0';
    //             }
    //         }
    //         break;
    //     }
    //     case ES_LOADING: {
    //         if (IsKeyPressed(KEY_ENTER)) {
    //             LoadLevel(editor->load_level_filename);
    //             editor->state = ES_IDLE;
    //         } else if (IsKeyPressed(KEY_BACKSPACE)) {
    //             uint32_t len = strlen(editor->load_level_filename);
    //             if (len > 0) {
    //                 editor->load_level_filename[len - 1] = '\0';
    //             }
    //         } else {
    //             int key = GetCharPressed();
    //             uint32_t name_len = strlen(editor->load_level_filename);
    //             if ((key >= 32) && (key <= 125) && (name_len < 255)) {  // 255 to allow for null terminator.
    //                 editor->load_level_filename[name_len] = (char)key;
    //                 editor->load_level_filename[name_len + 1] = '\0';
    //             }
    //         }
    //         break;
    //     }
    //     case ES_CAMERA: {
    //         if (IsKeyPressed(KEY_C)) {
    //             editor->state = ES_IDLE;
    //         } else {
    //             Camera3D *camera = &GetGameCamera();
    //             if (IsKeyPressed(KEY_ENTER)) {
    //                 GetCurLevel()->camera = *camera;
    //             }
    //             UpdateCamera(camera, CAMERA_FREE);
    //         }
    //         break;
    //     }
    //     default: {
    //         exit(EXIT_FAILURE);
    //     }
    // }

    // UpdateEntities.
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

static void EditorUpdate(float delta_time) { EditorUpdateEx(&Editor, delta_time); }
