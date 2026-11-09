// Unit/Jumbo build.

///////////////////////////////////////////////
// External libs below

// Only define once.
// TODO: Looks like raylib already defines this. Comment back in after removing raylib support.

#ifndef L_PLATFORM
#error "L_PLATFORM macro must be defined. Valid values: WIN32, RAYLIB."
#endif

#if L_PLATFORM == WIN32
#define STB_IMAGE_IMPLEMENTATION
#define PLATFORM_FILE "platform/win32_platform.c"

#elif L_PLATFORM == RAYLIB

#define PLATFORM_FILE "platform/raylib_platform.c"

#endif
#include "stb_image.h"
//
// Only define once.
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
//
//
//
// #include "glad/glad.h"
#include "glad/glad.h"

///////////////////////////////////////////////
// Project files below
#include "base.h"
//
#include "platform.h"
//
#include "base.c"
//
//
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "game.h"

//
#include "game.c"

// TODO: idk why but moving this to be any higher causing parsing error in game.c
#if L_PLATFORM == WIN32
#include "glad.c"

#endif

#include PLATFORM_FILE
