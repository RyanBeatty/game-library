// Unit/Jumbo build.

///////////////////////////////////////////////
// External libs below

// Only define once.
// TODO: Looks like raylib already defines this. Comment back in after removing raylib support.
#ifdef L_PLATFORM_WIN32
#define STB_IMAGE_IMPLEMENTATION
#define PLATFORM_FILE "platform/win32_platform.c"

#else

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
#include "base.c"
//
#include "platform.h"
//
//
#include "game.h"

//
#include "game.c"

//
#ifdef L_PLATFORM_WIN32

#include "glad.c"

#endif

#include PLATFORM_FILE
