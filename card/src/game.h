
#ifndef UNITY_BUILD
#include "shared.h"
#endif

#pragma once

typedef struct {
    bool down;
    // True if button ended up this frame and was down the last frame.
    // TODO: I'm not sure I can rely on this or if this is a good idea. Basically since we have decoupled the game
    // simulation updates from the renderer this will be true for multiple simulation ticks (but only for one GameTick).
    // Luckily I don't seem to use this too much really.
    bool pressed;
} GameButtonState;

typedef struct {
    bool connected;
    // The idea behind this is to virtualize a game controller. Instead of the game checking was the physical button A
    // pressed or was the physical Key WASD pressed, check was the "bottom button on the controller" pressed. The
    // platform layer then just fills out this struct and passes it to the game. This does mean that we only support a
    // limited number of actions in the game, but we can just keep filling this out (e.g. imagine the question " was the
    // Debug Menu Start button pressed").

    // This should be normalized between [-1, 1]
    Vec2 left_stick;

    // Where the virtualized cursor is (this could be the mouse or a cursor controlled by the controller)
    // Ideally this is in game space.
    Vec2 cursor_pos;

    union {
        GameButtonState buttons[2];
        struct {
            // GameButtonState action_up;
            GameButtonState action_down;
            GameButtonState action_left;
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
