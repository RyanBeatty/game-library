#ifndef UNITY_BUILD

#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#include "raylib.h"

#endif

typedef enum UIFlag {
    UIF_NONE = 0,
    UIF_GROW_VERTICAL = (1 << 0),
    UIF_CLICKABLE = (1 << 1),
    UIF_DISPLAY_TEXT = (1 << 2),
    UIF_ENTERABLE = (1 << 3),
    UIF_EDITABLE = (1 << 4)
} UIFlag;

#define UIFlagTestFlag(flags, flag) ((flags) & (flag))

typedef struct UIInput {
    float delta_time;
    int mouse_x;
    int mouse_y;
    bool mouse_down;
    bool mouse_released;
    int keys[100];
    uint32_t num_keys;
} UIInput;

typedef struct UIBox {
    char* label;

    char* display_text;
    char* edit_text;

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

static void UIUpdateInput(float delta_time) {
    UIReset();
    Input.delta_time = delta_time;
    Input.mouse_x = GetMouseX();
    Input.mouse_y = GetMouseY();
    bool is_mouse_down = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    Input.mouse_released = Input.mouse_down && !is_mouse_down;
    Input.mouse_down = is_mouse_down;

    memset(Input.keys, 0, sizeof(Input.keys) * sizeof(Input.keys[0]));
    Input.num_keys = 0;

    int key = GetKeyPressed();
    while (key != 0) {
        assert(Input.num_keys < 100);
        Input.keys[Input.num_keys] = key;
        ++Input.num_keys;
        key = GetKeyPressed();
    }
}

static bool InputIsKeyPressed(int key) {
    for (uint32_t i = 0; i < sizeof(Input.keys) * sizeof(Input.keys[0]); ++i) {
        if (Input.keys[i] == key) {
            return true;
        }
    }

    return false;
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

static bool UIBoxIsMouseInside(UIBox* box) {
    return (box->x <= Input.mouse_x && Input.mouse_x <= box->x + box->width) &&
           (box->y <= Input.mouse_y && Input.mouse_y <= box->y + box->height);
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

static UIBoxOutput MakeUIBoxOutput(UIBox* box) {
    if (UIBoxIsMouseInside(box)) {
        TraceLog(LOG_INFO, "%s hot", box->label);
        box->hot = true;
    } else {
        box->hot = false;
    }

    UIBoxOutput output = {0};
    if (box->active) {
        TraceLog(LOG_INFO, "%s active", box->label);

        if (UIBoxTestFlag(box, UIF_EDITABLE)) {
        }

        if (Input.mouse_released) {
            // TODO: Probably not accurate. Can probably run into strange situations where you press down  while
            // hot and release while hot on another widget.
            if (!box->hot) {
                box->active = false;
            }
            output.clicked = UIBoxTestFlag(box, UIF_CLICKABLE);
        }

        if (UIBoxTestFlag(box, UIF_ENTERABLE) && IsKeyPressed(KEY_ENTER)) {
            box->active = false;
            output.clicked = true;
        }
    } else if (box->hot) {
        if (Input.mouse_down) {
            box->active = true;
        }
    }

    return output;
}

static void DrawUIBox(UIBox* box) {
    static bool show_cursor = false;
    static float show_cursor_cooldown = 0.0f;

    if (State.enabled) {
        DrawRectangle(box->x, box->y, box->width, box->height, GREEN);
        if (box->display_text != NULL) {
            int pos_x = box->x + TEXT_WIDTH_BUFFER;
            int pos_y = box->y + TEXT_HEIGHT_BUFFER;
            int len = strlen(box->display_text);
            char* display_text = calloc(len + 2, sizeof(box->display_text[0]));
            display_text = strcat(display_text, box->display_text);
            if (UIBoxTestFlag(box, UIF_EDITABLE) && box->active) {
                show_cursor_cooldown += Input.delta_time;
                TraceLog(LOG_INFO, "%f %f", Input.delta_time, show_cursor_cooldown);
                if (show_cursor_cooldown >= 0.650f) {
                    show_cursor = !show_cursor;
                    show_cursor_cooldown = 0.0f;
                    breakpoint();
                }

                if (show_cursor) {
                    display_text[len] = '|';
                }
            }

            DrawText(display_text, pos_x, pos_y, State.font_size, BLACK);
            free(display_text);
        }
    }
}

//////////////////////////////////////////////////////////////////////

static void PushParent(char* label, int x, int y, int width, int height) {
    UIBox* box = MakeUIBox(label, x, y, width, height, UIF_GROW_VERTICAL);

    arrpush(State.parents, *box);

    if (State.enabled) {
        DrawRectangle(box->x, box->y, box->width, box->height, GRAY);
    }
};

static uint32_t GetTextPixelWidth(char* text) {
    return (uint32_t)(ceil(State.font_size * 1.33 * strlen(text)) + TEXT_WIDTH_BUFFER);
}
static uint32_t GetTextPixelHeight(char* text) { return (uint32_t)(ceil(State.font_size * 1.33) + TEXT_HEIGHT_BUFFER); }

static UIBoxOutput UIButton(char* label) {
    uint32_t width = GetTextPixelWidth(label);
    uint32_t height = GetTextPixelHeight(label);
    UIBox* box =
        MakeUIBox(label, TEXT_WIDTH_BUFFER, TEXT_HEIGHT_BUFFER, width, height, UIF_CLICKABLE | UIF_DISPLAY_TEXT);
    box->display_text = box->label;
    UIBoxOutput output = MakeUIBoxOutput(box);
    DrawUIBox(box);
    return output;
}

static UIBoxOutput UITextField(char* label, char* pre_edit) {
    uint32_t width = GetTextPixelWidth(pre_edit);
    uint32_t height = GetTextPixelHeight(pre_edit);
    UIBox* box = MakeUIBox(label, TEXT_WIDTH_BUFFER, TEXT_HEIGHT_BUFFER, width, height,
                           UIF_ENTERABLE | UIF_DISPLAY_TEXT | UIF_EDITABLE);
    if (!box->hot && !box->active) {
        box->display_text = pre_edit;
        free(box->edit_text);
        box->edit_text = (char*)calloc(strlen(pre_edit) + 1, sizeof(char));
        box->edit_text = strcat(box->edit_text, pre_edit);
    } else if (box->active) {
        box->display_text = box->edit_text;
    }
    UIBoxOutput output = MakeUIBoxOutput(box);
    DrawUIBox(box);
    return output;
}
