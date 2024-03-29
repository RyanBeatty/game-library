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

typedef struct UIState {
    int mx;
    int my;
    bool left_down;
    bool left_up;
    int font_size;
    Font font;

    uint16_t cursor;

    UIKey hot;
    UIKey active;
} UIState;

UIState State = {0};

static void UIStateSetHot(UIKey key) { State.hot = key; }
static void UIStateSetActive(UIKey key) { State.active = key; }
static void UIStateClearActive() { State.active = 0; }
static bool UIIsActive() { return State.active != 0; }
static bool UIKeyIsHot(UIKey key) { return State.hot == key; }
static bool UIKeyIsActive(UIKey key) { return State.active == key; }
static int UIGetFontSize() { return State.font_size; }
static Font UIGetFont() { return State.font; }
static int UIGetFontSpacing() { return State.font_size / State.font.baseSize; }

static void UIFrameStart() {
    State.mx = GetMouseX();
    State.my = GetMouseY();
    State.left_down = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    State.left_up = IsMouseButtonUp(MOUSE_BUTTON_LEFT);
    assert(!State.left_down || !State.left_up);

    State.font_size = 16;
    State.font = GetFontDefault();
    assert(State.font.texture.id != 0);

    State.hot = 0;
}

static bool InRect(int x, int y, int width, int height) {
    return (x <= State.mx && State.mx <= x + width) && (y <= State.my && State.my <= y + height);
}

static bool UIButton(char* label, int x, int y, int width, int height) {
    // TODO: Draw text.
    DrawRectangle(x, y, width, height, GRAY);
    // TODO: raylib MeasureText worries about fontsize < default font size...do I need to worry about this too?
    Vector2 text_size = MeasureTextEx(UIGetFont(), label, UIGetFontSize(), UIGetFontSpacing());
    float x_offset = (width - text_size.x) / 2;
    x_offset = x_offset < 0 ? 0 : x_offset;
    float y_offset = (height - text_size.y) / 2;
    y_offset = y_offset < 0 ? 0 : y_offset;
    DrawText(label, x + x_offset, y + y_offset, UIGetFontSize(), BLACK);

    UIKey key = UIKeyFromCStr(label);
    if (InRect(x, y, width, height)) {
        UIStateSetHot(key);
    }

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

static bool UIRadioButton(char* label, bool* state, int x, int y, int width, int height) {
    // TODO: Draw text.
    DrawRectangle(x, y, width, height, *state ? YELLOW : GRAY);
    // TODO: raylib MeasureText worries about fontsize < default font size...do I need to worry about this too?
    Vector2 text_size = MeasureTextEx(UIGetFont(), label, UIGetFontSize(), UIGetFontSpacing());
    float x_offset = (width - text_size.x) / 2;
    x_offset = x_offset < 0 ? 0 : x_offset;
    float y_offset = (height - text_size.y) / 2;
    y_offset = y_offset < 0 ? 0 : y_offset;
    DrawText(label, x + x_offset, y + y_offset, UIGetFontSize(), BLACK);

    UIKey key = UIKeyFromCStr(label);
    if (InRect(x, y, width, height)) {
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
static bool UITextField(char* label, char* edit, int x, int y, int width, int height) {
    UIKey key = UIKeyFromCStr(label);

    // TODO: Draw text.
    DrawRectangle(x, y, width, height, GRAY);

    char display_text[256] = {0};
    int len = strlen(edit);
    assert((sizeof(display_text) / sizeof(display_text[0])) > len);
    strcat(display_text, edit);
    // TODO: raylib MeasureText worries about fontsize < default font size...do I need to worry about this too?
    Vector2 text_size = MeasureTextEx(UIGetFont(), display_text, UIGetFontSize(), UIGetFontSpacing());
    float x_offset = (width - text_size.x) / 2;
    x_offset = x_offset < 0 ? 0 : x_offset;
    float y_offset = (height - text_size.y) / 2;
    y_offset = y_offset < 0 ? 0 : y_offset;
    DrawText(display_text, x + x_offset, y + y_offset, UIGetFontSize(), BLACK);

    if (InRect(x, y, width, height)) {
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
                TraceLog(LOG_INFO, "%s backspace", label);
                if (State.cursor != 0) {
                    --State.cursor;
                    edit[State.cursor] = '\0';
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
        }
    }
    return false;
}

static void UIFrameEnd() {}
