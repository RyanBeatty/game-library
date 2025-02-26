// Unit/Jumbo build.

///////////////////////////////////////////////
// External libs below

// #define WIN32_LEAN_AND_MEAN  // Exclude rarely-used components from Windows headers
// #define NOMINMAX             // Prevent Windows from defining min and max macros
#include <windows.h>
#include <xaudio2.h>

// #define RAYGUI_IMPLEMENTATION  // (If using raygui)
// #include "raylib.h"
// #include "raymath.h"

// Only define once.
#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"
// Only define once.
// TODO: Looks like raylib already defines this. Comment back in after removing raylib support.
// #define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
// Comment to prevent autolinting
#include <float.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include <array>
#include <chrono>
#include <thread>

#define Assert(expr)        \
    {                       \
        if (!(expr)) {      \
            breakpoint();   \
            assert((expr)); \
        }                   \
    }

void __attribute__((noinline)) breakpoint() {}

#define ArraySizeOf(x) (sizeof(x) / sizeof(x[0]))

///////////////////////////////////////////////
// Project files below
#include "main.c"
