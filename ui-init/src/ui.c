#ifndef UNITY_BUILD

#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#include "raylib.h"

#endif

typedef enum UIFlag { UIF_NONE = 0, UIF_GROW_VERTICAL = (1 << 0), UIF_COUNT = 2 } UIFlag;

#define UIFlagTestFlag(flags, flag) ((flags) & (flag))

typedef struct UIInputState {
} UIInputState;

typedef struct UIBox {
    int id;

    int x;
    int y;
    int width;
    int height;

    int sub_x;
    int sub_y;

    UIFlag flags;
} UIBox;

#define UIBoxTestFlag(box, flag) (UIFlagTestFlag((box)->flags, (flag)))

typedef struct UIBoxOutput {
    bool clicked;
} UIBoxOutput;

typedef struct UIState {
    bool enabled;

    UIBox* boxes;
    UIBox* parents;

    int font_size;
} UIState;

UIState State = {0};

#define TEXT_HEIGHT_BUFFER (5.0)
#define TEXT_WIDTH_BUFFER (5.0)

static void ToggleUI() {
    State.enabled = !State.enabled;
    TraceLog(LOG_INFO, "UI %s", State.enabled ? "Enabled" : "Disabled");
}

static void UIReset() {
    arrfree(State.boxes);
    State.boxes = NULL;
    arrfree(State.parents);
    State.parents = NULL;
    State.font_size = 22;
}

//////////////////////////////////////////////////////////////////////
// Parent stuff.

static void PushParent(int x, int y, int width, int height) {
    UIBox widget = {.x = x,
                    .y = y,
                    .width = width,
                    .height = height,
                    .sub_x = x + TEXT_WIDTH_BUFFER,
                    .sub_y = y + TEXT_HEIGHT_BUFFER,
                    .flags = UIF_GROW_VERTICAL};
    arrpush(State.parents, widget);

    if (State.enabled) {
        DrawRectangle(x, y, width, height, GRAY);
    }
};

static void PopParent() {
    assert(arrlenu(State.parents) > 0);
    arrpop(State.parents);
}

static UIBox* GetParent() {
    assert(arrlenu(State.parents) > 0);
    return &State.parents[arrlenu(State.parents) - 1];
}

//////////////////////////////////////////////////////////////////////

static uint32_t GetTextPixelWidth(char* text) {
    return (uint32_t)(ceil(State.font_size * 1.33 * strlen(text)) + TEXT_WIDTH_BUFFER);
}
static uint32_t GetTextPixelHeight(char* text) { return (uint32_t)(ceil(State.font_size * 1.33) + TEXT_HEIGHT_BUFFER); }

static uint32_t UIBoxReserveSpaceX(UIBox* box, uint32_t width) {
    uint32_t x = box->sub_x;
    return x;
}

static uint32_t UIBoxReserveSpaceY(UIBox* box, uint32_t height) {
    uint32_t y = box->sub_y;
    if (UIBoxTestFlag(box, UIF_GROW_VERTICAL)) {
        box->sub_y += height + TEXT_HEIGHT_BUFFER;
    }
    return y;
}

static UIBoxOutput UIButton(char* text) {
    UIBox* parent = GetParent();
    uint32_t width = GetTextPixelWidth(text);
    uint32_t height = GetTextPixelHeight(text);
    uint32_t x = UIBoxReserveSpaceX(parent, width);
    uint32_t y = UIBoxReserveSpaceY(parent, height);

    if (State.enabled) {
        DrawRectangle(x, y, width, height, GREEN);
        int pos_x = x + TEXT_WIDTH_BUFFER;
        int pos_y = y + TEXT_HEIGHT_BUFFER;
        DrawText(text, pos_x, pos_y, State.font_size, BLACK);
    }
}
