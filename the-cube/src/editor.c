#ifndef UNITY_BUILD
#include <errno.h>
#include <stdbool.h>

#include "entity.c"
#include "level.c"
#include "raylib.h"
#include "rlgl.h"

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

static void FocusMove(Focus *focus, Vector3 move) {
    LevelData *level_data = GetCurLevel();
    for (uint32_t i = 0; i < arrlenu(focus->entity_idxs); ++i) {
        Entity *entity = LevelDataGetEntity(level_data, focus->entity_idxs[i]);
        entity->position = Vector3Add(entity->position, move);
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

typedef enum EditorMode {
    SAVING = 0,
    LOADING,
    ENTITY_EDIT,
    DRAG_X,
    DRAG_Y,

    MODE_COUNT
} EditorMode;

typedef struct EditorState {
    // Index to entity we are focused on.
    Focus focus;
    char load_level_filename[256];

    bool modes[MODE_COUNT];

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

static bool IsEditorModeHot(EditorState *editor, EditorMode editor_bool) {
    for (EditorMode i = 0; i < MODE_COUNT; ++i) {
        if (i == editor_bool) {
            continue;
        }
        if (editor->modes[i]) {
            return false;
        }
    }
    return true;
}

static bool IsEditorModeActive(EditorState *editor, EditorMode editor_mode) {
    Assert(IsEditorModeHot(editor, editor_mode));
    return editor->modes[editor_mode];
}

static void EditorModeToggle(EditorState *editor, EditorMode editor_mode) {
    editor->modes[editor_mode] = !editor->modes[editor_mode];
}

static Vector3 ScreentoWorldRef(Vector2 pos, Matrix view, Matrix projection, int width, int height, Vector3 reference) {
    Matrix iview = MatrixInvert(view);
    Matrix iprojection = MatrixInvert(projection);

    Quaternion ref_clip = {reference.x, reference.y, reference.z, 1.0f};
    ref_clip = QuaternionTransform(ref_clip, view);
    ref_clip = QuaternionTransform(ref_clip, projection);

    float z = ref_clip.z / ref_clip.w;

    Vector4 clip = {((pos.x / width) - 0.5f) * 2.0f, ((pos.y / height) - 0.5f) * 2.0f, -z, 1.0f};
    clip = QuaternionTransform(clip, iprojection);
    clip = QuaternionTransform(clip, iview);
    Vector3 world = {clip.x, clip.y, clip.z};
    return world;
}

static void EditorFocusOff(EditorState *editor) {
    editor->modes[ENTITY_EDIT] = false;
    Focus *focus = EditorGetFocus(editor);
    Assert(focus != NULL);
    FocusDeInit(focus);
}

static void EditorUpdateEx(EditorState *editor, float delta_time) {
    static uint32_t color_idx = 0;
    static char saving_buffer[256] = {0};
    static char loading_buffer[256] = {0};
    static char editor_buffer[256] = {0};
    assert(EditorEnabled);

    LevelData *level = GetCurLevel();

    sprintf(editor_buffer, "Editing: %s", level->name);
    UIText("Editor greeting", editor_buffer);

    // entity editing.
    {
        Entity *clicked_entity = NULL;
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            Vector2 mouse_position = GetMousePosition();
            Ray mouse_click = GetMouseRay(mouse_position, level->camera);

            RayCollisionLevelData collision = GetRayCollisionLevelData(mouse_click, level);
            if (collision.collision.hit && !editor->modes[ENTITY_EDIT]) {
                clicked_entity = LevelDataGetEntity(level, collision.entity_idx);
            }
        }
        bool enter = IsKeyPressed(KEY_ENTER);
        if ((clicked_entity != NULL || enter) && IsEditorModeHot(editor, ENTITY_EDIT)) {
            if (!editor->modes[ENTITY_EDIT]) {
                editor->modes[ENTITY_EDIT] = true;
                EditorSetFocus(editor, clicked_entity);
            } else {
                editor->modes[ENTITY_EDIT] = false;
                Focus *focus = EditorGetFocus(editor);
                Assert(focus != NULL);
                FocusDeInit(focus);
            }
        }

        if (IsKeyPressed(KEY_ENTER) && IsEditorModeHot(editor, ENTITY_EDIT) && editor->modes[ENTITY_EDIT]) {
            EditorFocusOff(editor);
        }
        if (IsKeyPressed(KEY_BACKSPACE) && IsEditorModeHot(editor, ENTITY_EDIT) && editor->modes[ENTITY_EDIT]) {
            TraceLog(LOG_INFO, "delete");
            Focus *focus = EditorGetFocus(editor);
            Assert(focus != NULL);
            FocusDelete(focus);
            EditorFocusOff(editor);
        }

        if (editor->modes[ENTITY_EDIT]) {
            Focus *focus = EditorGetFocus(editor);
            Assert(focus != NULL);
            Camera3D camera = level->camera;

            Vector3 focus_pos_world = FocusGetPosition(focus);
            Vector2 focus_pos_screen = GetWorldToScreen(focus_pos_world, camera);

            int width = GetScreenWidth();
            int height = GetScreenHeight();
            Matrix view = MatrixLookAt(camera.position, camera.target, camera.up);
            double aspect = (double)width / (double)height;
            double top = camera.fovy / 2.0;
            double right = top * aspect;
            Matrix projection = MatrixOrtho(-right, right, -top, top, RL_CULL_DISTANCE_NEAR, RL_CULL_DISTANCE_FAR);

            Matrix iview = MatrixInvert(view);
            // Matrix iprojection = MatrixInvert(projection);

            // Quaternion ref_clip = {focus_pos_world.x, focus_pos_world.y, focus_pos_world.z, 1.0f};
            // ref_clip = QuaternionTransform(ref_clip, view);
            // ref_clip = QuaternionTransform(ref_clip, projection);

            // // float z = ref_clip.z / ref_clip.w;
            // float z = focus_pos_world.z;

            // Draw arrows.
            char labels[2][256] = {"x box", "y box"};
            Color colors[3] = {BLUE, GREEN, RED};
            Rectangle recs[2] = {{0, 0, 100, 10}, {-5, -100, 10, 100}};
            Vector3 arrows[3] = {(Vector3){1, 0, 0}, (Vector3){0, 1, 0}, (Vector3){0, 0, 1}};
            for (uint32_t i = 0; i < 2; ++i) {
                Vector2 pos = Vector2Add(focus_pos_screen, (Vector2){recs[i].x, recs[i].y});
                Color bg_color = colors[i];
                Color hot_color = bg_color;
                Color active_color = hot_color;
                UIBgColor(bg_color) {
                    UIHotColor(hot_color) {
                        UIActiveColor(active_color) {
                            UISignal signal = UIClickableRegion(labels[i], pos.x, pos.y, recs[i].width, recs[i].height);
                            if (signal.active) {
                                if (!Vector2Equals(signal.drag, Vector2Zero())) {
                                    // TODO: figure out how to translate mouse drag into 3d plane movement. the math
                                    // just aint mathing for me.
                                    // Vector2 new_focus_pos_screen = Vector2Add(focus_pos_screen, signal.drag);

                                    // Ray new_focus_pos_ray = GetMouseRay(new_focus_pos_screen, camera);

                                    // // Vector3 top_left = Vector3Unproject((Vector3){0, 0, z}, projection, view);
                                    // // Vector3 top_right = Vector3Unproject((Vector3){width, 0, z}, projection,
                                    // view);
                                    // // Vector3 bottom_left = Vector3Unproject((Vector3){0, height, z}, projection,
                                    // // view); Vector3 bottom_right =
                                    // //     Vector3Unproject((Vector3){width, height, z}, projection, view);
                                    // // RayCollision collision = GetRayCollisionQuad(new_focus_pos_ray, top_left,
                                    // // top_right,
                                    // //                                              bottom_left, bottom_right);
                                    // // RayCollision collision = GetRayCollisionQuad(
                                    // //     new_focus_pos_ray, (Vector3){-1000, 1000, z}, (Vector3){1000, 1000, z},
                                    // //     (Vector3){-1000, -1000, z}, (Vector3){1000, -1000, z});
                                    // // RayCollision collision = GetRayCollisionQuad(
                                    // //     new_focus_pos_ray, (Vector3){-1000, 1000, z}, (Vector3){1000, 1000, z},
                                    // //     (Vector3){-1000, -1000, z}, (Vector3){1000, -1000, z});
                                    // RayCollision collision = GetRayCollisionBox(
                                    //     new_focus_pos_ray,
                                    //     (BoundingBox){(Vector3){-1000, 1000, z}, (Vector3){1000, -1000, z}});
                                    // breakpoint();

                                    // // Calculate projection matrix from orthographic
                                    // projection = MatrixOrtho(-right, right, -top, top, RL_CULL_DISTANCE_NEAR,
                                    //                          RL_CULL_DISTANCE_FAR);
                                    // Vector3 new_focus_pos_world =
                                    //     ScreentoWorldRef(new_focus_pos_screen, view, projection, GetScreenWidth(),
                                    //                      GetScreenHeight(), focus_pos_world);
                                    // Vector3 move = Vector3Subtract(new_focus_pos_world, focus_pos_world);

                                    // FocusMove(focus, move);
                                    // // TraceLog(LOG_INFO, "movinggggg %s %f %f %f", labels[i], diff.x, diff.y,
                                    // diff.z); breakpoint();
                                }

                                // FocusMove(focus, (Vector3){drag.x, drag.y, 0});
                            }
                        }
                    }
                }
            }

            UIWindow("entity info", 0, 0, 100, GetScreenHeight()) {}

            // Keyboard movement.
            // right, left, down, up
            Vector3 dirs_camera[4] = {{1, 0, 0}, {-1, 0, 0}, {0, -1, 0}, {0, 1, 0}};
            Vector3 final_dir = {0, 0, 0};
            for (int i = KEY_RIGHT; i <= KEY_UP; ++i) {
                if (IsKeyPressed(i)) {
                    // Layout movement in camera space and then convert to world space and then subtract
                    // camera position to get the correct movement vector.
                    Vector3 dir_camera = dirs_camera[i - KEY_RIGHT];
                    Vector3 dir_world = Vector3Transform(dir_camera, iview);
                    dir_world = Vector3Subtract(dir_world, camera.position);
                    dir_world = Vector3Scale(dir_world, CubeSideLength);
                    final_dir = Vector3Add(final_dir, dir_world);
                }
            }
            FocusMove(focus, final_dir);
        }
    }

    // Level saving.
    {
        bool saving_toggled = IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_S);
        if (saving_toggled && IsEditorModeHot(editor, SAVING)) {
            EditorModeToggle(editor, SAVING);
            if (editor->modes[SAVING]) {
                strcat(saving_buffer, level->name);
            }
        }

        if (editor->modes[SAVING]) {
            UIRow("saving row") {
                bool button = UIButton("Save Level");
                bool edit_box = UIEditBox("saving text edit", saving_buffer);
                bool enter = IsKeyDown(KEY_ENTER);
                if (button || edit_box || enter) {
                    int len = strlen(level->name);
                    assert(len != 0);
                    memset(level->name, 0, len * sizeof(level->name[0]));
                    strcat(level->name, saving_buffer);
                    TraceLog(LOG_INFO, "Saving Level: %s", level->name);
                    SaveLevel(level);
                    editor->modes[SAVING] = false;
                }
            }
        } else {
            memset(saving_buffer, 0, sizeof(saving_buffer) * sizeof(saving_buffer[0]));
        }
    }

    // Level loading.
    {
        bool loading_toggled = IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_O);
        if (loading_toggled && IsEditorModeHot(editor, LOADING)) {
            EditorModeToggle(editor, LOADING);
            if (editor->modes[LOADING]) {
                strcat(loading_buffer, level->name);
            }
        }

        if (editor->modes[LOADING]) {
            UIRow("loading row") {
                bool button = UIButton("Load Level");
                bool edit_box = UIEditBox("loading text edit", loading_buffer);
                bool enter = IsKeyDown(KEY_ENTER);
                if (button || edit_box || enter) {
                    int len = strlen(level->name);
                    assert(len != 0);
                    memset(level->name, 0, len * sizeof(level->name[0]));
                    strcat(level->name, loading_buffer);
                    TraceLog(LOG_INFO, "loading Level: %s", level->name);
                    SaveLevel(level);
                    editor->modes[LOADING] = false;
                }
            }
        } else {
            memset(loading_buffer, 0, sizeof(loading_buffer) * sizeof(loading_buffer[0]));
        }
    }

    // UpdateEntities.
    {
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
}

static void EditorUpdate(float delta_time) { EditorUpdateEx(&Editor, delta_time); }
