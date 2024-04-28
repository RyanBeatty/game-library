#ifndef UNITY_BUILD

#include <math.h>
#include <raylib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#endif

#include <time.h>

static size_t GenerateRandomSeed() {
    // Get current time in seconds since Epoch
    time_t currentTime = time(NULL);

    // Generate a random seed using current time and clock ticks
    size_t seed = (size_t)(currentTime * 1000) + (size_t)clock();

    return seed;
}

static size_t GetSeed() {
    static size_t Seed = 0;
    while (Seed == 0) {
        Seed = GenerateRandomSeed();
    }
    return Seed;
}

typedef size_t UIKey;

static UIKey UIKeyFromCStr(char* str) {
    size_t key = stbds_hash_string(str, GetSeed());
    assert(key != 0);
    return key;
}

// defualt to growing along Y axis because thats more natural default for UIs.
typedef enum UIAxis { AXIS_Y = 0, AXIS_X, AXIS_COUNT } UIAxis;

typedef enum UISizeKind {
    SIZE_KIND_NULL = 0,
    SIZE_KIND_PIXELS,
    SIZE_KIND_TEXT_CONTENT,
    SIZE_KIND_PERCENT_OF_PARENT,

    SIZE_KIND_COUNT
} UISizeKind;

typedef struct UISize {
    UISizeKind kind;
    float value;
} UISize;

#define UISizePixel(px) ((UISize){.kind = SIZE_KIND_PIXELS, .value = (px)})
#define UISizePct(pct) ((UISize){.kind = SIZE_KIND_PERCENT_OF_PARENT, .value = (pct)})

typedef enum UIAlignment { ALIGN_CENTER = 0, ALIGN_LEFT, ALIGN_COUNT } UIAlignment;

typedef enum UIFill { FILL_WIDTH = 0, FILL_NONE, FILL_COUNT } UIFill;

typedef struct UIBox {
    union {
        struct {
            float y;
            float x;
        };

        float pos[AXIS_COUNT];
    };

    union {
        struct {
            float height;
            float width;
        };

        float size[AXIS_COUNT];
    };

    UISize semantic_size[AXIS_COUNT];

    UIKey key;
    UIKey neighbor_key;
    UIKey child_key;
    UIKey parent_key;

    struct UIBox* neighbor;
    struct UIBox* child;
    struct UIBox* parent;

    Color bg_color;
    int fontsize;
    UIAxis layout_axis;

    char* display;
} UIBox;

static void UIBoxAddChild(UIBox* parent, UIBox* child) {
    child->parent_key = parent->key;
    child->neighbor_key = parent->child_key;
    parent->child_key = child->key;
}

typedef struct UIState {
    int mx;
    int my;
    int dragx;
    int dragy;
    int resizex;
    int resizey;
    int resizew;
    int resizeh;
    bool left_down;
    bool left_up;
    int font_size;
    Font font;

    uint16_t cursor;
    bool show_cursor;
    float cursor_cooldown;

    struct {
        UIKey key;
        UIBox value;
    }* boxes;  // stb hash map.
    UIKey root;
    UIKey orig_root;

    UIAxis* layout_axis_stack;
    Color* bg_color_stack;

    UIKey hot;
    UIKey hot_to_be;
    UIKey active;
} UIState;

UIState State = {0};

static void UIPushLayoutAxis(UIAxis axis) { arrput(State.layout_axis_stack, axis); }
static void UIPopLayoutAxis() {
    assert(arrlenu(State.layout_axis_stack) != 0);
    arrpop(State.layout_axis_stack);
}
static UIAxis UIGetLayoutAxis() {
    size_t len = arrlenu(State.layout_axis_stack);
    assert(len != 0);
    return State.layout_axis_stack[len - 1];
}
static void UIPushBgColor(Color color) { arrput(State.bg_color_stack, color); }
static void UIPopBgColor() {
    size_t len = arrlenu(State.bg_color_stack);
    assert(len != 0);
    arrpop(State.bg_color_stack);
}
static Color UIGetBgColor() {
    size_t len = arrlenu(State.bg_color_stack);
    assert(len != 0);
    return State.bg_color_stack[len - 1];
}

static bool InRect(int x, int y, int width, int height) {
    return (x <= State.mx && State.mx <= x + width) && (y <= State.my && State.my <= y + height);
}
static void UIStateSetHot(UIKey key) { State.hot_to_be = key; }
static void UIStateSetActive(UIKey key) { State.active = key; }
static void UIStateClearActive() {
    State.active = 0;
    State.resizex = 0;
    State.resizey = 0;
    State.resizew = 0;
    State.resizey = 0;
    SetMouseCursor(MOUSE_CURSOR_DEFAULT);
}
static bool UIIsActive() { return State.active != 0; }
static bool UIShouldSetHot(int x, int y, int width, int height) { return !UIIsActive() && InRect(x, y, width, height); }
static bool UIKeyIsHot(UIKey key) { return State.hot == key; }
static bool UIKeyIsActive(UIKey key) { return State.active == key; }
static int UIGetFontSize() { return State.font_size; }
// static Font UIGetFont() { return State.font; }
static int UIGetFontSpacing() { return State.font_size / State.font.baseSize; }
static Vector2 UIMeasureText(char* text) {
    return MeasureTextEx(State.font, text, State.font_size, UIGetFontSpacing());
}
static Vector2 UIMeasureTextEx(char* text, int fontsize) {
    return MeasureTextEx(State.font, text, fontsize, fontsize / State.font.baseSize);
}
static UIBox* GetUIBox(UIKey key) {
    struct {
        UIKey key;
        UIBox value;
    }* res = hmgetp_null(State.boxes, key);
    if (res == NULL) {
        return NULL;
    }

    return &res->value;
}
static UIBox* UIGetParent() { return &hmgetp(State.boxes, State.root)->value; }
static void UIPushParent(UIBox* box) { State.root = box->key; }
static void UIPopParent() {
    UIBox* root = UIGetParent();
    assert(root != NULL);
    State.root = root->parent_key;
}

static void UIDrawRectangle(int x, int y, int width, int height, Color bg_color) { return; }
static void UIDrawText(UIBox parent, UIAlignment alignment, char* text) { return; }

static UIBox* MakeUIBox(char* label, UISize semantic_width, UISize semantic_height) {
    UIKey key = UIKeyFromCStr(label);
    ptrdiff_t idx = hmgeti(State.boxes, key);
    bool first_frame = idx == -1;
    if (first_frame) {
        UIBox box = {.key = key};
        hmput(State.boxes, key, box);
        idx = hmgeti(State.boxes, key);
        assert(idx != -1);
    }
    UIBox* box = &State.boxes[idx].value;

    // Reset per frame tree links
    box->child = box->neighbor = box->parent = NULL;

    UIBox* parent = UIGetParent();
    if (first_frame && parent != NULL) {
        UIBoxAddChild(parent, box);
    }

    box->semantic_size[AXIS_X] = semantic_width;
    box->semantic_size[AXIS_Y] = semantic_height;
    box->fontsize = UIGetFontSize();
    box->layout_axis = UIGetLayoutAxis();
    box->bg_color = UIGetBgColor();
    return box;
}

static void UIFrameStart(float delta_time) {
    static const float CURSOR_COOLDOWN_DEFAULT_SEC = 0.6;

    int mx = GetMouseX();
    int my = GetMouseY();
    State.dragx = mx - State.mx;
    State.dragy = my - State.my;
    State.mx = mx;
    State.my = my;
    State.left_down = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    State.left_up = IsMouseButtonUp(MOUSE_BUTTON_LEFT);
    assert(!State.left_down || !State.left_up);

    // Init stacks
    {
        State.layout_axis_stack = NULL;
        UIPushLayoutAxis(AXIS_Y);

        State.bg_color_stack = NULL;
        UIPushBgColor(GRAY);
    }

    State.font_size = 16;
    State.font = GetFontDefault();
    assert(State.font.texture.id != 0);

    State.cursor_cooldown += delta_time;
    if (State.cursor_cooldown >= CURSOR_COOLDOWN_DEFAULT_SEC) {
        State.show_cursor = !State.show_cursor;
        State.cursor_cooldown = 0.0f;
    }

    // TODO: Once we are pushing and saving multiple parents. Need to make sure we push the first
    // parent here.
    UIPushBgColor((Color){255, 255, 255, 0});
    UIBox* box = MakeUIBox("__window", (UISize){.kind = SIZE_KIND_PIXELS, .value = GetScreenWidth()},
                           (UISize){.kind = SIZE_KIND_PIXELS, .value = GetScreenHeight()});
    UIPopBgColor();
    State.root = box->key;
    State.orig_root = box->key;

    State.hot_to_be = 0;
    if (State.hot == 0) {
        SetMouseCursor(MOUSE_CURSOR_DEFAULT);
    }
}

static bool UIButtonEx(char* label, int x, int y, int width, int height) {
    TraceLog(LOG_DEBUG, "label %s %d %d %d %d", label, x, y, width, height);
    UIKey key = UIKeyFromCStr(label);
    if (UIShouldSetHot(x, y, width, height)) {
        UIStateSetHot(key);
    }

    if (UIKeyIsActive(key)) {
        TraceLog(LOG_DEBUG, "label %s is Active", label);
        if (State.left_up) {
            UIStateClearActive();
            return true;
        }
    } else if (UIKeyIsHot(key)) {
        TraceLog(LOG_DEBUG, "label %s is Hot. key %zu", label, key);
        if (State.left_down) {
            UIStateSetActive(key);
        }
    }
    return false;
}

static bool UIRadioButtonEx(char* label, bool* state, int x, int y, int width, int height) {
    // TODO: Draw text.
    UIDrawRectangle(x, y, width, height, *state ? YELLOW : GRAY);
    UIBox parent = {.x = x, .y = y, .width = width, .height = height};
    UIDrawText(parent, ALIGN_CENTER, label);

    UIKey key = UIKeyFromCStr(label);
    if (UIShouldSetHot(x, y, width, height)) {
        UIStateSetHot(key);
    }

    if (UIKeyIsActive(key)) {
        TraceLog(LOG_DEBUG, "label %s is Active", label);
        if (State.left_up) {
            *state = !(*state);
            UIStateClearActive();
            return true;
        }
    } else if (UIKeyIsHot(key)) {
        TraceLog(LOG_DEBUG, "label %s is Hot. key %zu", label, key);
        if (State.left_down) {
            UIStateSetActive(key);
        }
    }
    return false;
}

// TODO: We assume that edit has enough capacity to deal with any edits. This is a bad assumption
// consider passing in a max capacity here.
static bool UITextBoxEx(char* label, char* edit, int x, int y, int width, int height) {
    UIKey key = UIKeyFromCStr(label);

    UIDrawRectangle(x, y, width, height, GRAY);

    // // Draw Text.
    // {
    //     // TODO: raylib MeasureText worries about fontsize < default font size...do I need to worry about this too?

    //     char* display_text = NULL;
    //     int edit_len = strlen(edit);
    //     for (uint16_t i = 0; i < edit_len; ++i) {
    //         arrput(display_text, edit[i]);
    //     }
    //     if (UIKeyIsActive(key)) {
    //         if (State.show_cursor) {
    //             arrins(display_text, State.cursor, '|');
    //         }
    //     }
    //     arrput(display_text, '\0');

    //     UIBox parent = {.x = x, .y = y, .width = width, .height = height};
    //     UIDrawText(parent, ALIGN_LEFT, display_text);

    //     arrfree(display_text);
    //     display_text = NULL;
    // }

    if (UIShouldSetHot(x, y, width, height)) {
        UIStateSetHot(key);
    }

    if (UIKeyIsActive(key)) {
        TraceLog(LOG_DEBUG, "label %s is Active", label);

        int key = GetKeyPressed();
        if (key != 0) {
            bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);

            if (key >= 32 && key <= 126) {
                if (!shift && key >= 65 && key <= 90) {
                    key -= (int)('A' - 'a');
                }

                int len = strlen(edit);
                for (uint16_t i = len + 1; i > State.cursor; --i) {
                    edit[i] = edit[i - 1];
                }

                edit[State.cursor] = (char)key;
                ++State.cursor;
            } else if (IsKeyPressed(KEY_BACKSPACE)) {
                if (State.cursor != 0) {
                    int len = strlen(edit);
                    for (uint16_t i = State.cursor; i < len + 1; ++i) {
                        edit[i - 1] = edit[i];
                    }

                    --State.cursor;
                }
            } else if (IsKeyPressed(KEY_LEFT)) {
                if (State.cursor != 0) {
                    --State.cursor;
                }
            } else if (IsKeyPressed(KEY_RIGHT)) {
                if (edit[State.cursor] != '\0') {
                    ++State.cursor;
                }
            }

            if (key != 0) {
                State.show_cursor = true;
                State.cursor_cooldown = 0.0f;
            }

            if (IsKeyPressed(KEY_ENTER)) {
                UIStateClearActive();
                return true;
            }
        }
    } else if (UIKeyIsHot(key)) {
        TraceLog(LOG_DEBUG, "label %s is Hot. key %zu", label, key);
        if (State.left_down) {
            UIStateSetActive(key);
            State.cursor = strlen(edit);
            State.show_cursor = true;
            State.cursor_cooldown = 0.0f;
        }
    }
    return false;
}

static void BuildTree(UIBox* cur) {
    if (cur == NULL) {
        return;
    }

    cur->parent = GetUIBox(cur->parent_key);
    cur->neighbor = GetUIBox(cur->neighbor_key);
    cur->child = GetUIBox(cur->child_key);
    for (UIBox* child = cur->child; child != NULL; child = child->neighbor) {
        BuildTree(child);
    }

    // Reverse linked list to draw elements in correct order.
    UIBox* prev = NULL;
    UIBox* it = cur->child;
    UIBox* next = NULL;
    while (it != NULL) {
        next = it->neighbor;
        it->neighbor = prev;
        prev = it;
        it = next;
    }
    cur->child = prev;

    BuildTree(cur->neighbor);
}

static void CalculateFixedSizes(UIBox* cur, UIAxis axis) {
    if (cur == NULL) {
        return;
    }

    switch (cur->semantic_size[axis].kind) {
        case SIZE_KIND_PIXELS: {
            cur->size[axis] = cur->semantic_size[axis].value;
            break;
        }
        case SIZE_KIND_TEXT_CONTENT: {
            assert(cur->display != NULL);
            Vector2 size = UIMeasureTextEx(cur->display, cur->fontsize);
            float value = size.x;
            if (axis == AXIS_Y) {
                value = size.y;
            }
            cur->size[axis] = value;
            break;
        }
        case SIZE_KIND_PERCENT_OF_PARENT:
        case SIZE_KIND_NULL: {
            break;
        }
        default: {
            exit(1);
        }
    }

    for (UIBox* child = cur->child; child != NULL; child = child->neighbor) {
        CalculateFixedSizes(child, axis);
    }
    CalculateFixedSizes(cur->neighbor, axis);
}

static void CalculateUpwardSizes(UIBox* cur, UIAxis axis) {
    if (cur == NULL) {
        return;
    }

    switch (cur->semantic_size[axis].kind) {
        case SIZE_KIND_PERCENT_OF_PARENT: {
            assert(cur->parent != NULL);
            cur->size[axis] = cur->parent->size[axis] * cur->semantic_size[axis].value;
            break;
        }
        case SIZE_KIND_PIXELS:
        case SIZE_KIND_TEXT_CONTENT:
        case SIZE_KIND_NULL: {
            break;
        }
        default: {
            exit(1);
        }
    }
    for (UIBox* child = cur->child; child != NULL; child = child->neighbor) {
        CalculateUpwardSizes(child, axis);
    }
    CalculateUpwardSizes(cur->neighbor, axis);
}

static void LayoutUIBoxes(UIBox* cur, UIAxis axis) {
    if (cur == NULL) {
        return;
    }

    float offset = 0.0f;
    for (UIBox* child = cur->child; child != NULL; child = child->neighbor) {
        child->pos[axis] = cur->pos[axis] + offset;
        LayoutUIBoxes(child, axis);

        if (axis == cur->layout_axis) {
            offset += child->size[axis];
        }
    }
    LayoutUIBoxes(cur->neighbor, axis);
}

static void DrawUIBox(UIBox* cur) {
    breakpoint();
    BeginScissorMode(cur->x, cur->y, cur->width, cur->height);
    DrawRectangle(cur->x, cur->y, cur->width, cur->height, cur->bg_color);
    DrawRectangleLines(cur->x, cur->y, cur->width, cur->height, BLACK);

    // Draw Text.
    if (cur->display != NULL) {
        // TODO: raylib MeasureText worries about fontsize < default font size...do I need to worry
        // about this too?
        Vector2 text_size = UIMeasureTextEx(cur->display, cur->fontsize);

        // Alignment.
        float x_offset = 0.0;
        float y_offset = (cur->height - text_size.y) * 0.5;
        y_offset = y_offset < 0.0 ? 0.0 : y_offset;
        switch (ALIGN_CENTER) {
            case ALIGN_CENTER: {
                x_offset = (cur->width - text_size.x) * 0.5;
                x_offset = x_offset < 0.0 ? 0.0 : x_offset;
                break;
            }
            case ALIGN_LEFT: {
                x_offset = 0.0;
                break;
            }
            default: {
                exit(1);
            }
        }

        DrawText(cur->display, cur->x + x_offset, cur->y + y_offset, cur->fontsize, BLACK);
    }

    EndScissorMode();
}

static void DrawUI(UIBox* parent) {
    if (parent == NULL) {
        return;
    }

    DrawUIBox(parent);
    for (UIBox* child = parent->child; child != NULL; child = child->neighbor) {
        DrawUI(child);
    }
    DrawUI(parent->neighbor);
}

static void UIFrameEnd() {
    // Drawing.
    {
        assert(State.orig_root == State.root);
        UIBox* root = GetUIBox(State.root);
        assert(root != NULL);
        BuildTree(root);
        for (UIAxis axis = 0; axis < AXIS_COUNT; ++axis) {
            CalculateFixedSizes(root, axis);
            CalculateUpwardSizes(root, axis);
            LayoutUIBoxes(root, axis);
        }
        DrawUI(root);
    }

    State.hot = State.hot_to_be;

    // Deinit stacks.
    {
        arrfree(State.layout_axis_stack);
        arrfree(State.bg_color_stack);
    }
}

static bool UITextFieldEx(char* label, char* edit, int x, int y, int width, int height) {
    Vector2 size = UIMeasureText(label);
    UIBox parent = {.x = x, .y = y, .width = width, .height = height};
    UIDrawText(parent, ALIGN_LEFT, label);
    return UITextBoxEx(label, edit, x, y + size.y, width, height - size.y);
}

static bool UIButton(char* label) {
    UIBox* box = MakeUIBox(label, (UISize){SIZE_KIND_PERCENT_OF_PARENT, 1.0}, (UISize){SIZE_KIND_TEXT_CONTENT});
    box->display = label;
    box->bg_color = UIKeyIsHot(box->key) && !UIKeyIsActive(box->key) ? GREEN : UIGetBgColor();
    return UIButtonEx(label, box->x, box->y, box->width, box->height);
}

// static bool UIRadioButton(char* label, bool* state) {
//     Vector2 size = UIMeasureText(label);
//     int width = UIGetWidth(size.x);
//     int height = size.y + State.paddingy;

//     Vector2 pos = UIReserveSpace(width, height);
//     UIBox* box = MakeUIBox(label, pos.x, pos.y, width, height);
//     box->display = label;
//     box->bg_color = *state ? YELLOW : GRAY;
//     return UIRadioButtonEx(label, state, box->x, box->y, box->width, box->height);
// }

// static bool UITextBox(char* label, char* edit) {
//     Vector2 size = UIMeasureText(edit);
//     int width = UIGetWidth(size.x);
//     int height = size.y + State.paddingy;

//     Vector2 pos = UIReserveSpace(width, height);
//     UIBox* box = MakeUIBox(label, pos.x, pos.y, width, height);
//     box->display = NULL;

//     bool res = UITextBoxEx(label, edit, box->x, box->y, box->width, box->height);

//     {
//         // TODO: raylib MeasureText worries about fontsize < default font size...do I need to worry about this too?

//         int edit_len = strlen(edit);
//         for (uint16_t i = 0; i < edit_len; ++i) {
//             arrput(box->display, edit[i]);
//         }
//         if (UIKeyIsActive(box->key)) {
//             // TraceLog(LOG_INFO, "Activeeeee");
//             if (State.show_cursor) {
//                 // TraceLog(LOG_INFO, "cursor");

//                 arrins(box->display, State.cursor, '|');
//             }
//         }
//         arrput(box->display, '\0');
//     }
//     return res;
// }

// static bool UITextField(char* label, char* edit) {
//     Vector2 size = UIMeasureText(edit);
//     int width = UIGetWidth(size.x);
//     int height = size.y + State.paddingy;

//     size = UIMeasureText(label);
//     height += size.y + State.paddingy;

//     Vector2 pos = UIReserveSpace(width, height);
//     UIBox* box = MakeUIBox(label, pos.x, pos.y, width, height);
//     box->display = edit;
//     return UITextFieldEx(label, edit, box->x, box->y, box->width, box->height);
// }

//////////////////////////////////////////////////////////////////////////////
// Windows.

typedef enum UIWindowRect {
    RECT_FULL = 0,
    RECT_VISIBLE,
    RECT_DRAG,

    // Resize rects.

    // Resize NS
    RECT_NORTH,
    RECT_SOUTH,

    // Resize EW
    RECT_EAST,
    RECT_WEST,

    // Resize NWSE
    RECT_NW,
    RECT_SE,

    // Resize NESW
    RECT_NE,
    RECT_SW,

    RECT_COUNT
} UIWindowRect;

static int RectToCursor[RECT_COUNT] = {MOUSE_CURSOR_DEFAULT,     MOUSE_CURSOR_DEFAULT,     MOUSE_CURSOR_DEFAULT,
                                       MOUSE_CURSOR_RESIZE_NS,   MOUSE_CURSOR_RESIZE_NS,   MOUSE_CURSOR_RESIZE_EW,
                                       MOUSE_CURSOR_RESIZE_EW,   MOUSE_CURSOR_RESIZE_NWSE, MOUSE_CURSOR_RESIZE_NWSE,
                                       MOUSE_CURSOR_RESIZE_NESW, MOUSE_CURSOR_RESIZE_NESW};
static int RectToResizeX[RECT_COUNT] = {0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 1};
static int RectToResizeY[RECT_COUNT] = {0, 0, 1, 1, 0, 0, 0, 1, 0, 1, 0};
static int RectToResizeW[RECT_COUNT] = {0, 0, 0, 0, 0, 1, -1, -1, 1, 1, -1};
static int RectToResizeH[RECT_COUNT] = {0, 0, 0, -1, 1, 0, 0, -1, 1, -1, 1};

static Rectangle GetUIWindowRect(int x, int y, int width, int height, UIWindowRect kind) {
    static const int Padding = 20;  // 8;

    switch (kind) {
        case RECT_FULL: {
            return (Rectangle){
                .x = x - Padding, .y = y - Padding, .width = width + (Padding * 2), .height = height + (Padding * 2)};
        }
        case RECT_VISIBLE: {
            return (Rectangle){.x = x, .y = y, .width = width, .height = height};
        }
        case RECT_DRAG: {
            return (Rectangle){.x = x, .y = y, .width = width, .height = 20};
        }
        case RECT_NORTH: {
            return (Rectangle){.x = x, .y = y - Padding, .width = width, .height = Padding};
        }
        case RECT_SOUTH: {
            return (Rectangle){.x = x, .y = y + height, .width = width, .height = Padding};
        }
        case RECT_EAST: {
            return (Rectangle){.x = x + width, .y = y, .width = Padding, .height = height};
        }
        case RECT_WEST: {
            return (Rectangle){.x = x - Padding, .y = y, .width = Padding, .height = height};
        }
        case RECT_NW: {
            return (Rectangle){.x = x - Padding, .y = y - Padding, .width = Padding, .height = Padding};
        }
        case RECT_SE: {
            return (Rectangle){.x = x + width, .y = y + height, .width = Padding, .height = Padding};
        }
        case RECT_NE: {
            return (Rectangle){.x = x + width, .y = y - Padding, .width = Padding, .height = Padding};
        }
        case RECT_SW: {
            return (Rectangle){.x = x - Padding, .y = y + height, .width = Padding, .height = Padding};
        }
        default: {
            exit(1);
        }
    }
}

static bool UIWindowEx(char* label, int* x, int* y, int* width, int* height) {
    UIKey key = UIKeyFromCStr(label);

    Rectangle recs[RECT_COUNT] = {0};
    bool over[RECT_COUNT] = {0};
    for (int i = RECT_FULL; i < RECT_COUNT; ++i) {
        Rectangle rec = GetUIWindowRect(*x, *y, *width, *height, i);
        recs[i] = rec;
        over[i] = InRect(rec.x, rec.y, rec.width, rec.height);
    }

    if (UIShouldSetHot(recs[RECT_FULL].x, recs[RECT_FULL].y, recs[RECT_FULL].width, recs[RECT_FULL].height)) {
        UIStateSetHot(key);
    }

    UIDrawRectangle(recs[RECT_VISIBLE].x, recs[RECT_VISIBLE].y, recs[RECT_VISIBLE].width, recs[RECT_VISIBLE].height,
                    BLACK);
    UIDrawRectangle(recs[RECT_DRAG].x, recs[RECT_DRAG].y, recs[RECT_DRAG].width, recs[RECT_DRAG].height, BLUE);
    // Debug resize zones.
    if (false) {
        Color colors[5] = {RED, PURPLE, GREEN, YELLOW, PINK};
        for (int i = RECT_NORTH, c = 0; i < RECT_COUNT; ++i, ++c) {
            DrawRectangleRec(recs[i], colors[c % (sizeof(colors) / sizeof(colors[0]))]);
        }
    }

    if (UIKeyIsActive(key)) {
        TraceLog(LOG_DEBUG, "label %s is Active", label);
        if (State.left_up) {
            UIStateClearActive();
            return true;
        }
        *x += State.resizex * State.dragx;
        *y += State.resizey * State.dragy;
        *width += State.resizew * State.dragx;
        *height += State.resizeh * State.dragy;
        if (*width < 0) {
            *width = 0;
            *x -= State.resizex * State.dragx;
        }
        if (*height < 0) {
            *height = 0;
            *y -= State.resizey * State.dragy;
        }
    } else if (UIKeyIsHot(key)) {
        TraceLog(LOG_DEBUG, "label %s is Hot. key %zu", label, key);
        int cursor = MOUSE_CURSOR_DEFAULT;
        int resizex = 0;
        int resizey = 0;
        int resizew = 0;
        int resizeh = 0;
        // NOTE: We rely on the ordering of the enum to make sure these settings are okay.
        // We expect multiple matches, so need to remember last match/writer wins.
        for (int i = RECT_FULL; i < RECT_COUNT; ++i) {
            if (over[i]) {
                cursor = RectToCursor[i];
                resizex = RectToResizeX[i];
                resizey = RectToResizeY[i];
                resizew = RectToResizeW[i];
                resizeh = RectToResizeH[i];
            }
        }
        SetMouseCursor(cursor);

        if (State.left_down) {
            UIStateSetActive(key);
            State.resizex = resizex;
            State.resizey = resizey;
            State.resizew = resizew;
            State.resizeh = resizeh;
        }
    }
    return false;
}

static bool UIWindowPush(char* label, int* x, int* y, int* width, int* height) {
    UIBox* box = MakeUIBox(label, (UISize){SIZE_KIND_PIXELS, *width}, (UISize){SIZE_KIND_PIXELS, *height});
    box->x = *x;
    box->y = *y;

    // UIBox* parent = GetUIBox(box->parent_key);
    // assert(parent != NULL);
    // parent->child_key = box->neighbor_key;
    // box->neighbor_key = parent->neighbor_key;
    // parent->neighbor_key = box->key;
    // box->parent_key = parent->key;  // This will get modified once popped to point to parent->parent_key.
    State.root = box->key;

    return UIWindowEx(label, x, y, width, height);
}

static void UIWindowPop() {
    UIBox* root = GetUIBox(State.root);
    assert(root != NULL);
    UIBox* parent = GetUIBox(root->parent_key);
    assert(parent != NULL);
    // root->parent_key = parent->parent_key;
    State.root = parent->key;
}

static void UIRowBegin(char* label) {
    UIPushLayoutAxis(AXIS_X);
    UIBox* box = MakeUIBox(label, UISizePixel(100), UISizePixel(200));
    UIPopLayoutAxis();
    UIPushParent(box);
}

static void UIRowEnd() { UIPopParent(); }
