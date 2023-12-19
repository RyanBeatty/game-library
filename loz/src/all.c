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

#include <stdint.h>
#include <stdio.h>

///////////////////////////////////////////////
// Project files below
#include "main.c"
