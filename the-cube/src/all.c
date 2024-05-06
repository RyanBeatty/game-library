// Unit/Jumbo build.

///////////////////////////////////////////////
// External libs below

// Only define once.
#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"
// Comment to prevent autolinting

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

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

#define ONOFF(f) ((f) ? "On" : "Off")

#define GLSL_VERSION 330

///////////////////////////////////////////////
// Project files below

//
#include "entity.c"
//
#include "level.c"
//
#include "npc.c"
//
#include "transaction.c"
//
#include "player.c"
//
#include "serde.c"
//
#include "ui.c"
//
#include "editor.c"
// don't reorder
#include "main.c"
