// Unit/Jumbo build.

///////////////////////////////////////////////
// External libs below
#include "raylib.h"
#include "raymath.h"

// Only define once.
#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"
// Only define once.
// TODO: Looks like raylib already defines this. Comment back in after removing raylib support.
// #define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
//
#include "../external/cJSON/cJSON.h"
//
#include "glad/glad.h"
// Comment to prevent autolinting
#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
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

#define TestFlag(flags, flag) ((flags) & (flag))
#define FlagOn(flags, flag) ((flags) |= flag)
#define FlagOff(flags, flag) ((flags) &= ~(flag))

///////////////////////////////////////////////
// Project files below
#include "main.c"
