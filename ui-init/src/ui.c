#ifndef UNITY_BUILD

#include <stdlib.h>

#include "raylib.h"

#endif

bool UIEnabled = false;

static void ToggleUI() {
    UIEnabled = !UIEnabled;
    TraceLog(LOG_INFO, "UI %s", UIEnabled ? "Enabled" : "Disabled");
}

typedef struct UIInputState {
} UIInputState;

typedef struct UIState {
} UIState;

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

static void UIReset() {
    arrfree(Widgets);
    Widgets = NULL;
    arrfree(Parents);
    Parents = NULL;
}

static void PushParent(int x, int y, int width, int height) {
    UIWidget widget = {.x = x, .y = y, .width = width, .height = height};
    arrpush(Parents, widget);

    if (UIEnabled) {
        DrawRectangle(x, y, width, height, GRAY);
    }
};

static void PopParent() {
    assert(arrlenu(Parents) > 0);
    arrpop(Parents);
}

static UIWidget* GetParent() {
    assert(arrlenu(Parents) > 0);
    return &Parents[arrlenu(Parents) - 1];
}

static UIWidgetOutput UIButton(char* text, int x, int y, int width, int height) {}
