#ifndef UNITY_BUILD

#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#include "raylib.h"

#endif

typedef struct UIInputState {
} UIInputState;

typedef struct UIWidget {
    int id;

    int x;
    int y;
    int width;
    int height;
} UIWidget;

typedef struct UIWidgetOutput {
    bool clicked;
} UIWidgetOutput;

UIWidget* Widgets = NULL;

UIWidget* Parents = NULL;

typedef struct UIState {
    bool enabled;

    UIWidget* widgets;
    UIWidget* parents;

    int font_size;
} UIState;

UIState State = {0};

static void ToggleUI() {
    State.enabled = !State.enabled;
    TraceLog(LOG_INFO, "UI %s", State.enabled ? "Enabled" : "Disabled");
}

static void UIReset() {
    arrfree(State.widgets);
    State.widgets = NULL;
    arrfree(State.parents);
    State.parents = NULL;
    State.font_size = 22;
}

//////////////////////////////////////////////////////////////////////
// Parent stuff.

static void PushParent(int x, int y, int width, int height) {
    UIWidget widget = {.x = x, .y = y, .width = width, .height = height};
    arrpush(State.parents, widget);

    if (State.enabled) {
        DrawRectangle(x, y, width, height, GRAY);
    }
};

static void PopParent() {
    assert(arrlenu(State.parents) > 0);
    arrpop(State.parents);
}

static UIWidget* GetParent() {
    assert(arrlenu(State.parents) > 0);
    return &State.parents[arrlenu(State.parents) - 1];
}

//////////////////////////////////////////////////////////////////////

#define TEXT_HEIGHT_BUFFER (3.0)
#define TEXT_WIDTH_BUFFER (5.0)

static uint32_t GetTextPixelWidth(char* text) {
    return (uint32_t)(ceil(State.font_size * 1.33 * strlen(text)) + TEXT_WIDTH_BUFFER);
}
static uint32_t GetTextPixelHeight(char* text) { return (uint32_t)(ceil(State.font_size * 1.33) + TEXT_HEIGHT_BUFFER); }

static UIWidgetOutput UIButton(char* text) {
    if (State.enabled) {
        UIWidget* parent = GetParent();
        int size = strlen(text);
        uint32_t width = GetTextPixelWidth(text);
        uint32_t height = GetTextPixelHeight(text);

        DrawRectangle(parent->x + 10, parent->y + 10, width, height, GREEN);
    }
}
