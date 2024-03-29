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

#define RLIGHTS_IMPLEMENTATION
#include "rlights.h"

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
    const int screenWidth = 800;
    const int screenHeight = 450;
    static bool radio_button_1 = false;
    static char text_field[256] = {0};
    static char text_field2[256] = {0};

    strcat(text_field, "hello, world");
    strcat(text_field2, "foo");

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
        }
        // UIUpdateInput(delta_time);

        // if (IsKeyPressed(KEY_E)) {
        //     ToggleUI();
        // }

        // PushParent("parent1", 10, 10, 300, 400);

        // if (UIButton("Button").clicked) {
        //     TraceLog(LOG_INFO, "Button clicked");
        // }

        // if (UIButton("Another Button").clicked) {
        //     TraceLog(LOG_INFO, "Another Button Clicked");
        // }

        // if (UITextField("Text Field 1", "hello, workd").clicked) {
        //     TraceLog(LOG_INFO, "Text Field 1 Clicked");
        // }

        // PopParent();

        if (enabled) {
            UIFrameStart();
            if (UIButton("Button 1", 10, 10, 100, 100)) {
                TraceLog(LOG_INFO, "Button 1 clicked");
            }

            if (UIButton("Button 2", 10, 200, 200, 200)) {
                TraceLog(LOG_INFO, "Button 2 Clicked");
            }

            if (UIRadioButton("Radio Button", &radio_button_1, 300, 20, 100, 100)) {
                TraceLog(LOG_INFO, "Radio Button is %s", radio_button_1 ? "on" : "off");
            }

            if (UITextField("Text Field", text_field, 300, 150, 200, 50)) {
                TraceLog(LOG_INFO, "Text Field Entered");
            }

            if (UITextField("Text Field2", text_field2, 300, 250, 200, 150)) {
                TraceLog(LOG_INFO, "Text Field Entered");
            }

            UIFrameEnd();
        }

        BeginDrawing();

        ClearBackground(RAYWHITE);

        BeginMode3D(camera);

        // DrawModel(model, Vector3Zero(), 1.0f, WHITE);

        EndMode3D();

        // DrawFPS(?("Use keys [Y][R][G][B] to toggle lights", 10, 40, 20, DARKGRAY);

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    CloseWindow();  // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}