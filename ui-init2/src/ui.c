#ifndef UNITY_BUILD

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "raylib.h"

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

typedef struct UIBox {
    int x;
    int y;
    int width;
    int height;

    // For reserving space inside parent if has children.
    int cx;
    int cy;
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

    UIBox* parents;  // stb_arr.
    int spacing;
    int spacing_to_be;

    // TODO: Do something with this.
    UIAxis axis;

    int paddingx;  // Padding around button text and stuff.
    int paddingy;

    UIKey hot;
    UIKey hot_to_be;
    UIKey active;
} UIState;

UIState State = {0};

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
static UIBox* UIGetParent() { return &State.parents[arrlenu(State.parents) - 1]; }
static Rectangle UIGetClipRec() {
    UIBox* parent = UIGetParent();
    return (Rectangle){.x = parent->x, .y = parent->y, .width = parent->width, .height = parent->height};
}
static void UIDrawRectangle(int x, int y, int width, int height, Color bg_color) {
    Rectangle rec = UIGetClipRec();
    BeginScissorMode(rec.x, rec.y, rec.width, rec.height);
    DrawRectangle(x, y, width, height, bg_color);
    DrawRectangleLines(x, y, width, height, BLACK);
    EndScissorMode();
}
static void UISetSpacing(int spacing) {
    State.spacing = spacing;
    State.spacing_to_be = spacing;
}
static void UISetSpacing1(int spacing) {
    // Note: Will get overwritten by spacing_to_be at end of frame.
    State.spacing = spacing;
}

static Vector2 UIReserveSpace(int width, int height) {
    UIBox* parent = UIGetParent();
    int x = parent->x;

    parent->cy += State.spacing;
    int y = parent->y + parent->cy;
    parent->cy += height;
    return (Vector2){.x = x, .y = y};
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
    UIBox parent = {0};
    parent.x = 0;
    parent.y = 0;
    parent.width = GetScreenWidth();
    parent.height = GetScreenHeight();
    parent.cx = 0;
    parent.cy = 0;
    arrpush(State.parents, parent);

    State.spacing = 0;
    State.axis = AXIS_Y;
    State.paddingx = 10;
    State.paddingy = 5;

    State.hot_to_be = 0;
    if (State.hot == 0) {
        SetMouseCursor(MOUSE_CURSOR_DEFAULT);
    }
}

static bool UIButtonEx(char* label, int x, int y, int width, int height) {
    UIKey key = UIKeyFromCStr(label);
    if (UIShouldSetHot(x, y, width, height)) {
        UIStateSetHot(key);
    }

    UIDrawRectangle(x, y, width, height, UIKeyIsHot(key) && !UIKeyIsActive(key) ? GREEN : GRAY);
    // TODO: raylib MeasureText worries about fontsize < default font size...do I need to worry about this too?
    Vector2 text_size = UIMeasureText(label);
    float x_offset = (width - text_size.x) * 0.5;
    x_offset = x_offset < 0 ? 0 : x_offset;
    float y_offset = (height - text_size.y) * 0.5;
    y_offset = y_offset < 0 ? 0 : y_offset;
    BeginScissorMode(x, y, width, height);
    DrawText(label, x + x_offset, y + y_offset, UIGetFontSize(), BLACK);
    EndScissorMode();

    if (UIKeyIsActive(key)) {
        TraceLog(LOG_INFO, "label %s is Active", label);
        if (State.left_up) {
            UIStateClearActive();
            return true;
        }
    } else if (UIKeyIsHot(key)) {
        TraceLog(LOG_INFO, "label %s is Hot. key %zu", label, key);
        if (State.left_down) {
            UIStateSetActive(key);
        }
    }
    return false;
}

static bool UIRadioButtonEx(char* label, bool* state, int x, int y, int width, int height) {
    // TODO: Draw text.
    UIDrawRectangle(x, y, width, height, *state ? YELLOW : GRAY);
    // TODO: raylib MeasureText worries about fontsize < default font size...do I need to worry about this too?
    Vector2 text_size = UIMeasureText(label);
    float x_offset = (width - text_size.x) * 0.5;
    x_offset = x_offset < 0 ? 0 : x_offset;
    float y_offset = (height - text_size.y) * 0.5;
    y_offset = y_offset < 0 ? 0 : y_offset;
    BeginScissorMode(x, y, width, height);
    DrawText(label, x + x_offset, y + y_offset, UIGetFontSize(), BLACK);
    EndScissorMode();

    UIKey key = UIKeyFromCStr(label);
    if (UIShouldSetHot(x, y, width, height)) {
        UIStateSetHot(key);
    }

    if (UIKeyIsActive(key)) {
        TraceLog(LOG_INFO, "label %s is Active", label);
        if (State.left_up) {
            *state = !(*state);
            UIStateClearActive();
            return true;
        }
    } else if (UIKeyIsHot(key)) {
        TraceLog(LOG_INFO, "label %s is Hot. key %zu", label, key);
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

    // Draw Text.
    {
        // TODO: raylib MeasureText worries about fontsize < default font size...do I need to worry about this too?
        Vector2 edit_text_size = UIMeasureText(edit);
        float x_offset = 5;
        x_offset = x_offset < 0 ? 0 : x_offset;
        float y_offset = (height - edit_text_size.y) * 0.5;
        y_offset = y_offset < 0 ? 0 : y_offset;

        char* display_text = NULL;
        int edit_len = strlen(edit);
        for (uint16_t i = 0; i < edit_len; ++i) {
            arrput(display_text, edit[i]);
        }
        if (UIKeyIsActive(key)) {
            if (State.show_cursor) {
                arrins(display_text, State.cursor, '|');
            }
        }
        arrput(display_text, '\0');

        // TODO: Improve clipping rect calculation to take into account cursor position.
        BeginScissorMode(x, y, width, height);
        DrawText(display_text, x + x_offset, y + y_offset, UIGetFontSize(), BLACK);
        EndScissorMode();
        arrfree(display_text);
        display_text = NULL;
    }

    if (UIShouldSetHot(x, y, width, height)) {
        UIStateSetHot(key);
    }

    if (UIKeyIsActive(key)) {
        TraceLog(LOG_INFO, "label %s is Active", label);

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
        TraceLog(LOG_INFO, "label %s is Hot. key %zu", label, key);
        if (State.left_down) {
            UIStateSetActive(key);
            State.cursor = strlen(edit);
            State.show_cursor = true;
            State.cursor_cooldown = 0.0f;
        }
    }
    return false;
}

static void UIFrameEnd() {
    State.hot = State.hot_to_be;
    State.spacing = State.spacing_to_be;
    arrfree(State.parents);
    State.parents = NULL;
}

static bool UITextFieldEx(char* label, char* edit, int x, int y, int width, int height) {
    Vector2 size = UIMeasureText(label);
    UIDrawRectangle(x, y, width, size.y, GRAY);
    BeginScissorMode(x, y, width, height);
    DrawText(label, x, y, UIGetFontSize(), BLACK);
    EndScissorMode();
    return UITextBoxEx(label, edit, x, y + size.y, width, height - size.y);
}

static bool UIButton(char* label) {
    Vector2 size = UIMeasureText(label);
    int width = UIGetParent()->width;
    int height = size.y + State.paddingy;

    Vector2 pos = UIReserveSpace(width, height);

    return UIButtonEx(label, pos.x, pos.y, width, height);
}

static bool UIRadioButton(char* label, bool* state) {
    Vector2 size = UIMeasureText(label);
    int width = UIGetParent()->width;
    int height = size.y + State.paddingy;

    Vector2 pos = UIReserveSpace(width, height);
    return UIRadioButtonEx(label, state, pos.x, pos.y, width, height);
}

static bool UITextBox(char* label, char* edit) {
    Vector2 size = UIMeasureText(edit);
    int width = UIGetParent()->width;
    int height = size.y + State.paddingy;

    Vector2 pos = UIReserveSpace(width, height);
    return UITextBoxEx(label, edit, pos.x, pos.y, width, height);
}

static bool UITextField(char* label, char* edit) {
    Vector2 size = UIMeasureText(edit);
    int width = UIGetParent()->width;
    int height = size.y + State.paddingy;

    size = UIMeasureText(label);
    height += size.y + State.paddingy;

    Vector2 pos = UIReserveSpace(width, height);
    return UITextFieldEx(label, edit, pos.x, pos.y, width, height);
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
        TraceLog(LOG_INFO, "label %s is Active", label);
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
        }
        if (*height < 0) {
            *height = 0;
            *y -= State.resizey * State.dragy;
        }
    } else if (UIKeyIsHot(key)) {
        TraceLog(LOG_INFO, "label %s is Hot. key %zu", label, key);
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

static bool UIPushWindow(char* label, int* x, int* y, int* width, int* height) {
    UIBox parent = {.x = *x, .y = *y, .width = *width, .height = *height, .cx = 0, .cy = 0};
    arrpush(State.parents, parent);
    Rectangle rec = GetUIWindowRect(*x, *y, *width, *height, RECT_DRAG);
    UIReserveSpace(rec.width, rec.height);
    return UIWindowEx(label, x, y, width, height);
}

static void UIPopWindow() {
    assert(arrlenu(State.parents) != 0);
    (void)arrpop(State.parents);
}