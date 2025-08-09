#ifndef UNITY_BUILD
#include <shared.h>

#include "platform.h"

#endif

#include "raylib.h"

LCORE void PlatformLog(LogLevel level, const char* fmt, ...) {
    char buffer[256] = {0};

    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    TraceLog(LOG_INFO, "%s", buffer);
}

LCORE i32 PlatformKeyToRaylibKey(LKey key) {
    switch (key) {
        case LKEY_UP: {
            return KEY_UP;
        }
        case LKEY_DOWN: {
            return KEY_DOWN;
        }
        case LKEY_LEFT: {
            return KEY_LEFT;
        }
        case LKEY_RIGHT: {
            return KEY_RIGHT;
        }
        case LKEY_ENTER: {
            return KEY_ENTER;
        }
        case LKEY_X: {
            return KEY_X;
        }
        case LKEY_Z: {
            return KEY_Z;
        }
        case LKEY_1: {
            return KEY_ONE;
        }
        case LKEY_LCONTROL: {
            return KEY_LEFT_CONTROL;
        }
        default: {
            PlatformLog(LLOG_ERROR, "Unknown key num=%d hex=%x c=%c\n", key, key, (char)key);
            return LKEY_NONE;
        }
    }
}

LCORE bool PlatformKeyPressed(LKey key) { return IsKeyPressed(PlatformKeyToRaylibKey(key)); }
LCORE bool PlatformKeyReleased(LKey key) { return IsKeyReleased(PlatformKeyToRaylibKey(key)); }
LCORE bool PlatformKeyDown(LKey key) { return IsKeyDown(PlatformKeyToRaylibKey(key)); }

u32 PlatformScreenWidth() { return GetScreenWidth(); }
u32 PlatformScreenHeight() { return GetScreenHeight(); }

LCORE void PlatformInit(u32 width, u32 height, u32 target_fps) {
    InitWindow(width, height, "Game");
    SetWindowPosition(100, 100);
    // SetWindowState(FLAG_WINDOW_UNDECORATED);
    SetConfigFlags(FLAG_VSYNC_HINT);
    SetTargetFPS(target_fps);
}

void PlatformInitAudio(byte* buffer, isize cap) {}
void PlatformClearSounds() {}
// Can load canonical wav files: http://soundfile.sapp.org/doc/WaveFormat/
SoundId PlatformLoadSound(const char* filename) { return 0; }
// limit = 0, means no limit
void PlatformPlaySoundLimit(SoundId id, SoundFlag flags, u32 limit) {}

u32 PlatformSoundCountPlaying(SoundId id) { return 0; }
void PlatformStopSound(SoundId) {}

int main(void) {
    // Initialize dyanmic memory pool.
    isize cap = GB(4);
    byte* buffer = PlatformAllocate(cap);
    Arena perm = NewArena(buffer, cap);
    GameInit(perm);

    while (!WindowShouldClose()) {
        BeginDrawing();

        GameTick(GetFrameTime());

        EndDrawing();
    }

    CloseWindow();

    return 0;
}
