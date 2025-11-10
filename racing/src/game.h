
#ifndef UNITY_BUILD
#include "shared.h"
#endif

#pragma once

typedef struct {
    bool down;
    bool pressed;
} GameButtonState;

typedef struct {
    bool connected;
    // The idea behind this is to virtualize a game controller. Instead of the game checking was the physical button A
    // pressed or was the physical Key WASD pressed, check was the "bottom button on the controller" pressed. The
    // platform layer then just fills out this struct and passes it to the game. This does mean that we only support a
    // limited number of actions in the game, but we can just keep filling this out (e.g. imagine the question " was the
    // Debug Menu Start button pressed").
    union {
        GameButtonState buttons[1];
        struct {
            // GameButtonState action_up;
            GameButtonState action_down;
            // GameButtonState action_left;
            // GameButtonState action_right;
        };
    };
} GameControllerState;

typedef struct {
    // 0 is keyboard + mouse, rest are controllers
    GameControllerState controllers[5];
} GameInput;

void GameInit();

void GameTick(f32 dt, GameInput* input);
