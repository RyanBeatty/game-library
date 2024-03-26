// Unit/Jumbo build.

///////////////////////////////////////////////
// External libs below

// Only define once.
#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"
// Comment to prevent autolinting

#define Assert(expr)        \
    {                       \
        if (!(expr)) {      \
            breakpoint();   \
            assert((expr)); \
        }                   \
    }

void __attribute__((noinline)) breakpoint() {}

///////////////////////////////////////////////
// Project files below
#include "raylib.h"
#include "rlights.h"
//
#include "ui.c"
//
#include "main.c"
