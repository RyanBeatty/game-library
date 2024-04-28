/*******************************************************************************************
 *
 *   raylib [shaders] example - basic lighting
 *
 *   NOTE: This example requires raylib OpenGL 3.3 or ES2 versions for shaders support,
 *         OpenGL 1.1 does not support shaders, recompile raylib to OpenGL 3.3 version.
 *
 *   NOTE: Shaders used in this example are #version 330 (OpenGL 3.3).
 *
 *   Example originally created with raylib 3.0, last time updated with raylib 4.2
 *
 *   Example contributed by Chris Camacho (@codifies) and reviewed by Ramon Santamaria (@raysan5)
 *
 *   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
 *   BSD-like license that allows static linking with closed source software
 *
 *   Copyright (c) 2019-2024 Chris Camacho (@codifies) and Ramon Santamaria (@raysan5)
 *
 ********************************************************************************************/

#include "raylib.h"
#include "raymath.h"

#if defined(PLATFORM_DESKTOP)
#define GLSL_VERSION 330
#else  // PLATFORM_ANDROID, PLATFORM_WEB
#define GLSL_VERSION 100
#endif

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void) {
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 1366;
    const int screenHeight = 768;
    static bool radio_button_1 = false;
    static char text_box[256] = {0};
    static char text_box2[256] = {0};
    // static int wx = 0;
    // static int wy = 200;
    // static int ww = 200;
    // static int wh = 300;

    strcat(text_box, "hello, world");
    strcat(text_box2, "foo");

    SetTraceLogLevel(getenv("DEBUG") != NULL ? LOG_DEBUG : LOG_INFO);
    SetConfigFlags(FLAG_MSAA_4X_HINT);  // Enable Multi Sampling Anti Aliasing 4x (if available)
    InitWindow(screenWidth, screenHeight, "raylib [shaders] example - basic lighting");

    // Define the camera to look into our 3d world
    Camera camera = {0};
    camera.position = (Vector3){0.0f, 7.0f, 12.0f};  // Camera position
    camera.target = (Vector3){0.0f, 0.5f, 0.0f};     // Camera looking at point
    camera.up = (Vector3){0.0f, 1.0f, 0.0f};         // Camera up vector (rotation towards target)
    camera.fovy = 45.0f;                             // Camera field-of-view Y
    camera.projection = CAMERA_PERSPECTIVE;          // Camera projection type

    Model model = LoadModelFromMesh(GenMeshCube(3, 3, 3));
    model.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = BLUE;

    SetTargetFPS(60);  // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    bool enabled = false;

    // Main game loop
    while (!WindowShouldClose())  // Detect window close button or ESC key
    {
        float delta_time = GetFrameTime();

        if (!UIIsActive() && IsKeyPressed(KEY_E)) {
            enabled = !enabled;
            TraceLog(LOG_INFO, "UI %s", enabled ? "Enabled" : "Disabled");
        }

        BeginDrawing();

        DrawFPS(10, 10);

        ClearBackground(RAYWHITE);

        if (enabled) {
            UIFrameStart(delta_time);

            UIButton("button 1");
            UIButton("button 12");

            // UIWindowPush("w1", &wx, &wy, &ww, &wh);
            UIRowBegin("row1");
            UIPushBgColor(RED);
            UIButton("button 3");
            UIPopBgColor();

            UIPushActiveColor(BLUE);
            UIButton("buttonnnnnnn");
            UIPopActiveColor();

            UIRowEnd();

            // UIWindowPop();

            // if (UIPushWindow("Window", &wx, &wy, &ww, &wh)) {
            //     TraceLog(LOG_INFO, "Window active");
            // }

            // UIButton("button center");

            // UIButton("button center2");

            // UISetSpacing(30);

            // UIButton("button center 3");

            // UISetSpacing(0);
            // UIButton("Button Center 4");

            // if (UITextBox("Text Box", text_box)) {
            //     TraceLog(LOG_INFO, "Text Box Entered");
            // }

            // if (UITextField("Health", text_box2)) {
            //     TraceLog(LOG_INFO, "Health Entered");
            // }

            // UISetFill(FILL_NONE);
            // if (UIRadioButton("Radio Button", &radio_button_1)) {
            //     TraceLog(LOG_INFO, "Radio Button is %s", radio_button_1 ? "on" : "off");
            // }
            // UISetFill(FILL_WIDTH);

            // UIPopWindow();

            // UISetSpacing1(50);
            // UISetFill(FILL_NONE);
            // UISetPaddingX(40);
            // UISetFontSize(26);
            // UIButton("Start Game");
            // UISetSpacing(20);
            // UIButton("Continue");

            UIFrameEnd();
        }

        BeginMode3D(camera);

        EndMode3D();

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    CloseWindow();  // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}