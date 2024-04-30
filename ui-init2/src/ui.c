#ifndef UNITY_BUILD

#include <math.h>
#include <raylib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#endif

#include <time.h>

#define UIDeferLoop(begin, end) for (int _i_ = ((begin), 0); !_i_; _i_ += 1, (end))

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
    SIZE_KIND_SUM_OF_CHILDREN,

    SIZE_KIND_COUNT
} UISizeKind;

typedef struct UISize {
    UISizeKind kind;
    float value;
    float strictness;  // [0.0, 1.0]
} UISize;

#define UISizePx(px, strct) ((UISize){.kind = SIZE_KIND_PIXELS, .value = (px), .strictness = (strct)})
#define UISizePct(pct, strct) ((UISize){.kind = SIZE_KIND_PERCENT_OF_PARENT, .value = (pct), .strictness = (strct)})
#define UISizeSum() ((UISize){.kind = SIZE_KIND_SUM_OF_CHILDREN, .strictness = 0.0f})
#define UISizeTxt(strct) ((UISize){.kind = SIZE_KIND_TEXT_CONTENT, .strictness = (strct)})

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
    const char* key_str;
    UIKey neighbor_key;
    UIKey child_key;
    UIKey last_child_key;
    UIKey parent_key;

    uint32_t nchildren;

    struct UIBox* neighbor;
    struct UIBox* child;
    struct UIBox* last_child;
    struct UIBox* parent;

    Color bg_color;
    Color font_color;
    int fontsize;
    UIAxis layout_axis;

    // Table info.
    uint32_t nrows;
    uint32_t ncols;

    uint32_t nframes;

    char* display;
} UIBox;

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
    Color* active_color_stack;
    Color* font_color_stack;

    UIKey hot;
    UIKey hot_to_be;
    UIKey active;
} UIState;

UIState State = {0};

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

static void UIBoxAddChild(UIBox* parent, UIBox* child) {
    child->parent_key = parent->key;

    if (parent->child_key == 0 || parent->last_child_key == 0) {
        parent->child_key = child->key;
        parent->last_child_key = child->key;
    } else {
        UIBox* last_child = GetUIBox(parent->last_child_key);
        last_child->neighbor_key = child->key;
        parent->last_child_key = child->key;
        // Might be necessary for things that rebalance tree after node creation (i.e. tables).
        child->neighbor_key = 0;
    }

    // child->neighbor_key = parent->child_key;
    // parent->child_key = child->key;
    ++parent->nchildren;
}

static void UIBoxChildPopLeft(UIBox* parent) {
    assert(parent->child_key != 0 && parent->last_child_key != 0);
    UIBox* child = GetUIBox(parent->child_key);
    assert(child != NULL);
    if (parent->child_key == parent->last_child_key) {
        parent->child_key = 0;
        parent->last_child_key = 0;
    } else {
        parent->child_key = child->neighbor_key;
    }
    child->neighbor_key = 0;
    child->neighbor_key = 0;
    --parent->nchildren;
}

static void UIPushLayoutAxis(UIAxis axis) { arrput(State.layout_axis_stack, axis); }
static void UIPopLayoutAxis() {
    assert(arrlenu(State.layout_axis_stack) != 0);
    (void)arrpop(State.layout_axis_stack);
}
static UIAxis UIGetLayoutAxis() {
    size_t len = arrlenu(State.layout_axis_stack);
    assert(len != 0);
    return State.layout_axis_stack[len - 1];
}
#define UILayoutAxis(axis) UIDeferLoop(UIPushLayoutAxis(axis), UIPopLayoutAxis())

static void UIPushBgColor(Color color) { arrput(State.bg_color_stack, color); }
static void UIPopBgColor() {
    size_t len = arrlenu(State.bg_color_stack);
    assert(len != 0);
    (void)arrpop(State.bg_color_stack);
}
static Color UIGetBgColor() {
    size_t len = arrlenu(State.bg_color_stack);
    assert(len != 0);
    return State.bg_color_stack[len - 1];
}
#define UIBgColor(color) UIDeferLoop(UIPushBgColor(color), UIPopBgColor())

static void UIPushActiveColor(Color color) { arrput(State.active_color_stack, color); }
static void UIPopActiveColor() {
    size_t len = arrlenu(State.active_color_stack);
    assert(len != 0);
    (void)arrpop(State.active_color_stack);
}
static Color UIGetActiveColor() {
    size_t len = arrlenu(State.active_color_stack);
    assert(len != 0);
    return State.active_color_stack[len - 1];
}
#define UIActiveColor(color) UIDeferLoop(UIPushActiveColor(color), UIPopActiveColor())

static void UIPushFontColor(Color color) { arrput(State.font_color_stack, color); }
static void UIPopFontColor() {
    size_t len = arrlenu(State.font_color_stack);
    assert(len != 0);
    (void)arrpop(State.font_color_stack);
}
static Color UIGetFontColor() {
    size_t len = arrlenu(State.font_color_stack);
    assert(len != 0);
    return State.font_color_stack[len - 1];
}
#define UIFontColor(color) UIDeferLoop(UIPushFontColor(color), UIPopFontColor())

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
static Vector2 UIMeasureTextEx(char* text, int fontsize) {
    return MeasureTextEx(State.font, text, fontsize, fontsize / State.font.baseSize);
}
static UIBox* UIGetParent() { return &hmgetp(State.boxes, State.root)->value; }
static void UIPushParent(UIBox* box) { State.root = box->key; }
static void UIPopParent() {
    UIBox* root = UIGetParent();
    assert(root != NULL);
    State.root = root->parent_key;
}

static UIBox* MakeUIBox(char* label, UISize semantic_width, UISize semantic_height) {
    UIKey key = UIKeyFromCStr(label);
    ptrdiff_t idx = hmgeti(State.boxes, key);
    bool first_frame = idx == -1;
    if (first_frame) {
        UIBox box = {0};
        box.key = key;
        size_t len = strlen(label);
        char* str = (char*)calloc(len + 1, sizeof(char));
        box.key_str = strcat(str, label);
        hmput(State.boxes, key, box);
        idx = hmgeti(State.boxes, key);
        assert(idx != -1);
    }
    UIBox* box = &State.boxes[idx].value;

    // Reset per frame tree links
    box->child = box->neighbor = box->parent = box->last_child = NULL;

    UIBox* parent = UIGetParent();
    if (first_frame && parent != NULL) {
        UIBoxAddChild(parent, box);
    }

    box->semantic_size[AXIS_X] = semantic_width;
    box->semantic_size[AXIS_Y] = semantic_height;

    // Set Stack-based atts.
    box->font_color = UIGetFontColor();
    box->fontsize = UIGetFontSize();
    box->layout_axis = UIGetLayoutAxis();
    box->bg_color = UIGetBgColor();
    ++box->nframes;
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

        State.active_color_stack = NULL;
        UIPushActiveColor(GREEN);

        State.font_color_stack = NULL;
        UIPushFontColor(WHITE);
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
    UIBox* box = NULL;
    UIBgColor(((Color){255, 255, 255, 0})) {
        box = MakeUIBox("__window", (UISize){.kind = SIZE_KIND_PIXELS, .value = GetScreenWidth()},
                        (UISize){.kind = SIZE_KIND_PIXELS, .value = GetScreenHeight()});
    }
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
static bool UIEditBoxEx(char* label, char* edit, int x, int y, int width, int height) {
    UIKey key = UIKeyFromCStr(label);

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
    cur->last_child = GetUIBox(cur->last_child_key);
    for (UIBox* child = cur->child; child != NULL; child = child->neighbor) {
        BuildTree(child);
    }
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
        case SIZE_KIND_SUM_OF_CHILDREN:
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
        case SIZE_KIND_SUM_OF_CHILDREN:
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
}

static void CalculateDownwardSizes(UIBox* cur, UIAxis axis) {
    if (cur == NULL) {
        return;
    }

    float children_sum_size = 0.0f;
    float children_max_size = 0.0f;
    for (UIBox* child = cur->child; child != NULL; child = child->neighbor) {
        CalculateDownwardSizes(child, axis);
        children_sum_size += child->size[axis];
        children_max_size = fmax(children_max_size, child->size[axis]);
    }
    switch (cur->semantic_size[axis].kind) {
        case SIZE_KIND_SUM_OF_CHILDREN: {
            cur->size[axis] = axis == cur->layout_axis ? children_sum_size : children_max_size;
            break;
        }
        case SIZE_KIND_PERCENT_OF_PARENT:
        case SIZE_KIND_PIXELS:
        case SIZE_KIND_TEXT_CONTENT:
        case SIZE_KIND_NULL: {
            break;
        }
        default: {
            exit(1);
        }
    }
}

static void SolveConstraints(UIBox* cur, UIAxis axis) {
    if (cur == NULL || cur->nchildren == 0.0f) {
        return;
    }

    float children_sum_size = 0.0f;
    float children_max_size = 0.0f;
    for (UIBox* child = cur->child; child != NULL; child = child->neighbor) {
        SolveConstraints(child, axis);
        children_sum_size += child->size[axis];
        children_max_size = fmaxf(child->size[axis], children_max_size);
    }

    float size = axis == cur->layout_axis ? children_sum_size : children_max_size;
    if (size > cur->size[axis]) {
        float remaining = size - cur->size[axis];
        float negative_size = remaining / cur->nchildren;
        for (UIBox* child = cur->child; child != NULL; child = child->neighbor) {
            child->size[axis] -= negative_size;
        }
    }
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
}

static void DrawUIBox(UIBox* cur) {
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

        DrawText(cur->display, cur->x + x_offset, cur->y + y_offset, cur->fontsize, cur->font_color);
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
            CalculateDownwardSizes(root, axis);
            SolveConstraints(root, axis);
            LayoutUIBoxes(root, axis);
        }
        DrawUI(root);
    }

    State.hot = State.hot_to_be;

    // Deinit stacks.
    {
        arrfree(State.layout_axis_stack);
        arrfree(State.bg_color_stack);
        arrfree(State.active_color_stack);
        arrfree(State.font_color_stack);
    }
}

static void UISpacer(char* label, UISize width, UISize height) {
    Color white = WHITE;
    white.a = 0;
    UIBgColor(white) { MakeUIBox(label, width, height); }
}

static void UIText(char* label) {
    UIBox* box = MakeUIBox(label, UISizePct(1.0f, 0.0f), UISizeTxt(0.0f));
    box->display = label;
}
static bool UIButton(char* label) {
    UIBox* box = MakeUIBox(label, UISizePct(1.0f, 0.0f), UISizeTxt(0.0f));
    box->display = label;
    box->bg_color = UIKeyIsHot(box->key) && !UIKeyIsActive(box->key) ? UIGetActiveColor() : UIGetBgColor();
    return UIButtonEx(label, box->x, box->y, box->width, box->height);
}

static bool UIRadioButton(char* label, bool* state) {
    UIBox* box = MakeUIBox(label, UISizePct(1.0f, 0.0f), UISizeTxt(0.0f));
    box->display = label;
    box->bg_color = *state ? UIGetActiveColor() : UIGetBgColor();
    return UIRadioButtonEx(label, state, box->x, box->y, box->width, box->height);
}

static bool UIEditBox(char* label, char* edit) {
    UIBox* box = MakeUIBox(label, UISizePct(1.0f, 0.0f), UISizeTxt(0.0f));
    bool res = UIEditBoxEx(label, edit, box->x, box->y, box->width, box->height);

    {
        // TODO: raylib MeasureText worries about fontsize < default font size...do I need to worry about this too?
        if (box->display != NULL) {
            arrfree(box->display);
            box->display = NULL;
        }
        int edit_len = strlen(edit);
        for (uint16_t i = 0; i < edit_len; ++i) {
            arrput(box->display, edit[i]);
        }
        if (UIKeyIsActive(box->key)) {
            // TraceLog(LOG_INFO, "Activeeeee");
            if (State.show_cursor) {
                // TraceLog(LOG_INFO, "cursor");

                arrins(box->display, State.cursor, '|');
            }
        }
        arrput(box->display, '\0');
    }
    return res;
}

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

    // UIDrawRectangle(recs[RECT_VISIBLE].x, recs[RECT_VISIBLE].y, recs[RECT_VISIBLE].width, recs[RECT_VISIBLE].height,
    //                 BLACK);
    // UIDrawRectangle(recs[RECT_DRAG].x, recs[RECT_DRAG].y, recs[RECT_DRAG].width, recs[RECT_DRAG].height, BLUE);
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

// static bool UIWindowPush(char* label, int* x, int* y, int* width, int* height) {
//     UIBox* box = MakeUIBox(label, (UISize){SIZE_KIND_PIXELS, *width}, (UISize){SIZE_KIND_PIXELS, *height});
//     box->x = *x;
//     box->y = *y;

//     // UIBox* parent = GetUIBox(box->parent_key);
//     // assert(parent != NULL);
//     // parent->child_key = box->neighbor_key;
//     // box->neighbor_key = parent->neighbor_key;
//     // parent->neighbor_key = box->key;
//     // box->parent_key = parent->key;  // This will get modified once popped to point to parent->parent_key.
//     State.root = box->key;

//     return UIWindowEx(label, x, y, width, height);
// }

// static void UIWindowPop() {
//     UIBox* root = GetUIBox(State.root);
//     assert(root != NULL);
//     UIBox* parent = GetUIBox(root->parent_key);
//     assert(parent != NULL);
//     // root->parent_key = parent->parent_key;
//     State.root = parent->key;
// }

static UIBox* UIRowBegin(char* label) {
    UIBox* box = NULL;
    UILayoutAxis(AXIS_X) { box = MakeUIBox(label, UISizePct(1.0, 0.0), UISizeSum()); }
    UIPushParent(box);
    return box;
}

static void UIRowEnd() { UIPopParent(); }
#define UIRow(label) UIDeferLoop(UIRowBegin(label), UIRowEnd())

static void UITableBegin(char* label, uint32_t nrows, uint32_t ncols) {
    UIBox* box = MakeUIBox(label, UISizePct(1.0f, 0.0f), UISizeSum());
    box->nrows = nrows;
    box->ncols = ncols;
    UIPushParent(box);
}

static void UITableEnd() {
    UIBox* table = UIGetParent();
    assert(table != NULL);
    if (table->nframes == 1) {
        // Rewrite the children pushed in table's scope such that row nodes are inserted where necessary.
        assert(table->nchildren == table->nrows * table->ncols);
        for (uint32_t i = 0; i < table->nrows; ++i) {
            char num[256] = {0};
            sprintf(num, "%d", i);
            size_t num_len = strlen(num);
            size_t len = strlen(table->key_str);
            char* row_str = (char*)calloc(len + num_len + 1, sizeof(char));
            row_str = strcat(row_str, table->key_str);
            row_str = strcat(row_str, num);
            UIRow(row_str) {
                UIKey row_key = UIKeyFromCStr(row_str);
                UIBox* row = GetUIBox(row_key);
                assert(row != NULL);
                UIBox* cur = GetUIBox(table->child_key);
                UIBox* next = GetUIBox(cur->neighbor_key);
                for (uint32_t j = 0; j < table->ncols; ++j) {
                    assert(cur != NULL);
                    UIBoxChildPopLeft(table);
                    UIBoxAddChild(row, cur);
                    cur = next;
                    next = GetUIBox(next->neighbor_key);
                }
            }
        }
    }

    UIPopParent();
}
