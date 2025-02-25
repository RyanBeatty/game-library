#ifndef UNITY_BUILD
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "raylib.h"
#include "raymath.h"

#endif

////////////////////////////////////////////////////////////
// Core types.

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float f32;
typedef double f64;

#define F32_MAX FLT_MAX
// Min value a float can differ by.
#define F32_EPSILON FLT_EPSILON

typedef f32 rad;
typedef f32 deg;

#define LPI ((f32)3.1415926535)
#define LTAU ((f32)6.2831853071)
#define LPHI ((f32)1.6180339887)  // golden ratio

#define Deg2Rad(d) ((d) * LTAU / 360.0f)
#define Rad2Deg(r) ((r) * 360.0f / LTAU)
#define RGB01(v) ((v) / 255.0f)

int main(void) {
    const int screenWidth = 800;
    const int screenHeight = 450;

    // InitWindow(screenWidth, screenHeight, "raylib [core] example - basic window");

    // SetTargetFPS(60);  // Set our game to run at 60 frames-per-second

    // // Must call at least once.
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) return hr;

    IXAudio2 *m_xaudio2{};

    // Create the main audio interface.
    if (FAILED(XAudio2Create(&m_xaudio2, 0, XAUDIO2_DEFAULT_PROCESSOR))) {
        printf("Failed to create XAudio2 instance\n");
    }

    // while (!WindowShouldClose())  // Detect window close button or ESC key
    // {
    //     BeginDrawing();

    //     ClearBackground(RAYWHITE);

    //     DrawText("Congrats! You created your first window!", 190, 200, 20, LIGHTGRAY);

    //     EndDrawing();
    // }

    // CloseWindow();

    return 0;
}
