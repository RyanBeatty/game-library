// Unit/Jumbo build.

///////////////////////////////////////////////
// External libs below

// Only define once.
// TODO: Looks like raylib already defines this. Comment back in after removing raylib support.
// #define STB_IMAGE_IMPLEMENTATION
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
#include "shared.h"
//
//
#ifdef L_PLATFORM_WIN32

#include "glad.c"
#include "shared.c"
//
#include "platform/win32_platform.c"

#else
#include "raylib.h"
//
#include "audio.h"
//
#include "platform.h"

//
#include "main.c"

//
#include "platform/raylib_platform.c"

#endif
