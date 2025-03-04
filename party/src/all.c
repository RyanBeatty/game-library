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
#include <stddef.h>
#include <stdio.h>
#include <time.h>

///////////////////////////////////////////////
// Project files below
#include "shared.h"
//
#include "main.c"
