#ifndef UNITY_BUILD

#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#include "raylib.h"

#endif

typedef enum UIFlag { UIF_NONE = 0, UIF_GROW_VERTICAL = (1 << 0), UIF_CLICKABLE = (1 << 1) } UIFlag;

#define UIFlagTestFlag(flags, flag) ((flags) & (flag))

typedef struct UIInput {
    int mouse_x;
    int mouse_y;
    bool mouse_down;
} UIInput;

typedef struct UIBox {
    char* label;

    int x;
    int y;
    int width;
    int height;

    int sub_x;
    int sub_y;

    UIFlag flags;

    bool hot;
    bool active;
} UIBox;

#define UIBoxTestFlag(box, flag) (UIFlagTestFlag((box)->flags, (flag)))

typedef struct UIBoxOutput {
    bool clicked;
} UIBoxOutput;

typedef struct UIState {
    bool enabled;

    struct {
        char* key;
        UIBox value;
    } * widgets;
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
    arrfree(State.parents);
    State.parents = NULL;
    State.font_size = 22;
}

//////////////////////////////////////////////////////////////////////
// UIInpu

UIInput Input = {0};

static void UIUpdateInput() {
    UIReset();
    Input.mouse_x = GetMouseX();
    Input.mouse_y = GetMouseY();
    Input.mouse_down = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
}

//////////////////////////////////////////////////////////////////////
// Parent stuff.

static void PopParent() {
    assert(arrlenu(State.parents) > 0);
    arrdel(State.parents, arrlenu(State.parents) - 1);
}

static UIBox* GetParent() {
    if (arrlenu(State.parents) == 0) {
        return NULL;
    }
    return &State.parents[arrlenu(State.parents) - 1];
}

//////////////////////////////////////////////////////////////////////
// UIBox.

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

static UIBox* MakeUIBox(char* label, int x, int y, int width, int height, UIFlag flags) {
    ptrdiff_t idx = hmgeti(State.widgets, label);
    if (idx == -1) {
        UIBox* parent = GetParent();
        if (parent != NULL) {
            x = UIBoxReserveSpaceX(parent, width) + x;
            y = UIBoxReserveSpaceY(parent, height) + y;
        }

        UIBox box = {.x = x, .y = y, .width = width, .height = height, .flags = flags, .sub_x = x, .sub_y = y};
        box.label = (char*)calloc(strlen(label) + 1, sizeof(char));
        box.label = strcat(box.label, label);
        hmput(State.widgets, label, box);
        idx = hmgeti(State.widgets, label);
    }
    return &State.widgets[idx].value;
}

//////////////////////////////////////////////////////////////////////

static void PushParent(char* label, int x, int y, int width, int height) {
    UIBox* box = MakeUIBox(label, x, y, width, height, UIF_GROW_VERTICAL);

    arrpush(State.parents, *box);

    if (State.enabled) {
        DrawRectangle(box->x, box->y, box->width, box->height, GRAY);
    }
};

static bool UIBoxIsMouseInside(UIBox* box) {
    return (box->x <= Input.mouse_x && Input.mouse_x <= box->x + box->width) &&
           (box->y <= Input.mouse_y && Input.mouse_y <= box->y + box->height);
}

static uint32_t GetTextPixelWidth(char* text) {
    return (uint32_t)(ceil(State.font_size * 1.33 * strlen(text)) + TEXT_WIDTH_BUFFER);
}
static uint32_t GetTextPixelHeight(char* text) { return (uint32_t)(ceil(State.font_size * 1.33) + TEXT_HEIGHT_BUFFER); }

static UIBoxOutput UIButton(char* label) {
    uint32_t width = GetTextPixelWidth(label);
    uint32_t height = GetTextPixelHeight(label);
    UIBox* box = MakeUIBox(label, TEXT_WIDTH_BUFFER, TEXT_HEIGHT_BUFFER, width, height, UIF_CLICKABLE);

    if (UIBoxIsMouseInside(box)) {
        TraceLog(LOG_INFO, "%s hot", label);
        box->hot = true;
    } else {
        box->hot = false;
    }

    UIBoxOutput output = {0};
    if (box->active) {
        TraceLog(LOG_INFO, "%s active", label);
        if (!Input.mouse_down) {
            box->active = false;
            output.clicked = UIBoxTestFlag(box, UIF_CLICKABLE);
        }
    } else if (box->hot) {
        if (Input.mouse_down) {
            box->active = true;
        }
    }

    return output;
}

static UIBoxOutput UITextField(char* text, char* pre_edit, char** edit_out) {
    UIBoxOutput output = {0};
    return output;
}
