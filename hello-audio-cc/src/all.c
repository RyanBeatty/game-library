// Unit/Jumbo build.

///////////////////////////////////////////////
// External libs below

// Only define once.
#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"
// Only define once.
// TODO: Looks like raylib already defines this. Comment back in after removing raylib support.
// #define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
//
// Comment to prevent autolinting
#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#define ArraySizeOf(x) (sizeof(x) / sizeof(x[0]))

#define TestFlag(flags, flag) ((flags) & (flag))
#define FlagOn(flags, flag) ((flags) |= flag)
#define FlagOff(flags, flag) ((flags) &= ~(flag))

///////////////////////////////////////////////
// Project files below
//
#include "audio.h"
//
#include "main.c"
