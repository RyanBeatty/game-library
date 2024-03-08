// Unit/Jumbo build.

///////////////////////////////////////////////
// External libs below

// Only define once.
#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"
// Comment to prevent autolinting

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

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

//
#include "camera.c"
//
#include "entity.c"
//
#include "level.c"
//
#include "npc.c"
//
#include "player.c"
//
#include "serde.c"
//
// #include "nuklear_shim.c"
//
#include "editor.c"
// don't reorder
#include "main.c"
