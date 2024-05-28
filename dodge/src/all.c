// Unit/Jumbo build.

///////////////////////////////////////////////
// External libs below
#include "raylib.h"
#include "raymath.h"

// Only define once.
#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"
// Comment to prevent autolinting
#include <stdint.h>
#include <time.h>

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
