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

static UIKey UIKeyFromCStr(char* str) { return stbds_hash_string(str, GetSeed()); }

typedef struct UIState {
    int mx;
    int my;

    UIKey hot;
} UIState;

UIState State = {0};

static void UIStateSetHot(UIKey key) { State.hot = key; }

static void UIFrameStart() {
    State.mx = GetMouseX();
    State.my = GetMouseY();
}

static bool InRect(int x, int y, int width, int height) {
    return (x <= State.mx && State.mx <= x + width) && (y <= State.my && State.my <= y + height);
}

static bool UIButton(char* label, int x, int y, int width, int height) {
    DrawRectangle(x, y, width, height, GRAY);

    UIKey key = UIKeyFromCStr(label);
    if (InRect(x, y, width, height)) {
        TraceLog(LOG_INFO, "label %s is Hot. key %zu", label, key);
        UIStateSetHot(key);
    }

    return false;
}

static void UIFrameEnd() {}
