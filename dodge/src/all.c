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
#define SetFlag(flags, flag) ((flags) |= flag)
#define FlagOff(flags, flag) ((flags) &= ~(flag))

///////////////////////////////////////////////
// Project files below
#include "main.c"
