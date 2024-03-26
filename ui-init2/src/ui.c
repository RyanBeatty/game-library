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

    UIKey hot;
    UIKey active;
} UIState;

UIState State = {0};

static void UIStateSetHot(UIKey key) { State.hot = key; }
static void UIStateSetActive(UIKey key) { State.active = key; }
static void UIStateClearActive() { State.active = 0; }
static bool UIKeyIsHot(UIKey key) { return State.hot == key; }
static bool UIKeyIsActive(UIKey key) { return State.active == key; }

static void UIFrameStart() {
    State.mx = GetMouseX();
    State.my = GetMouseY();
    State.left_down = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    State.left_up = IsMouseButtonUp(MOUSE_BUTTON_LEFT);
    assert(!State.left_down || !State.left_up);

    State.hot = 0;
}

static bool InRect(int x, int y, int width, int height) {
    return (x <= State.mx && State.mx <= x + width) && (y <= State.my && State.my <= y + height);
}

static bool UIButton(char* label, int x, int y, int width, int height) {
    DrawRectangle(x, y, width, height, GRAY);

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

static void UIFrameEnd() {}
