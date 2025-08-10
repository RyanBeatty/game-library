#ifndef UNITY_BUILD
#include <shared.h>

#include "platform.h"

#endif

#include "raylib.h"

#define MAX_SOUNDS 32

typedef struct {
    Arena perm;

    // Need sounds and muscis because you can't loop a sound but you can't play multiple musics.
    Sound sounds[MAX_SOUNDS];
    Music musics[MAX_SOUNDS];
    bool loaded[MAX_SOUNDS];

    SoundId streams[MAX_SOUNDS];
    bool playing[MAX_SOUNDS];
} RaylibPlatformState;

static RaylibPlatformState RPState = {0};

#define GetRaylibPlatformState() (&RPState)

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

void PlatformInitAudio(byte* buffer, isize cap) { InitAudioDevice(); }
void PlatformClearSounds() {
    RaylibPlatformState* rpstate = GetRaylibPlatformState();
    for (i32 i = NULLSOUND + 1; i < ArraySizeOf(rpstate->loaded); ++i) {
        if (rpstate->loaded[i]) {
            Sound sound = rpstate->sounds[i];
            StopSound(sound);
            UnloadSound(sound);
            Music music = rpstate->musics[i];
            StopMusicStream(music);
            UnloadMusicStream(music);
            rpstate->loaded[i] = false;
        }
    }
}
// Can load canonical wav files: http://soundfile.sapp.org/doc/WaveFormat/
SoundId PlatformLoadSound(const char* filename) {
    PlatformLog(LLOG_INFO, "Loading sound: %s", filename);
    RaylibPlatformState* rpstate = GetRaylibPlatformState();

    for (i32 i = NULLSOUND + 1; i < ArraySizeOf(rpstate->sounds); ++i) {
        if (!rpstate->loaded[i]) {
            rpstate->loaded[i] = true;
            Sound sound = LoadSound(filename);
            Assert((sound.frameCount > 0) &&         // Validate frame count
                   (sound.stream.buffer != NULL) &&  // Validate stream buffer
                   (sound.stream.sampleRate > 0) &&  // Validate sample rate is supported
                   (sound.stream.sampleSize > 0) &&  // Validate sample size is supported
                   (sound.stream.channels > 0));
            rpstate->sounds[i] = sound;
            rpstate->musics[i] = LoadMusicStream(filename);
            return (SoundId)i;
        }
    }
    Assert(false);
    return NULLSOUND;
}
// limit = 0, means no limit
void PlatformPlaySoundLimit(SoundId id, SoundFlag flags, u32 limit) {
    RaylibPlatformState* rpstate = GetRaylibPlatformState();

    u32 count = 0;
    i32 choice = NULLSOUND;
    for (i32 i = NULLSOUND + 1; i < ArraySizeOf(rpstate->streams); ++i) {
        if (!rpstate->playing[i]) {
            choice = i;
        } else if (rpstate->streams[i] == id) {
            ++count;
        }
    }

    if (choice != NULLSOUND && (limit == 0 || limit < count)) {
        rpstate->streams[choice] = id;
        rpstate->playing[choice] = true;
        if (FlagOn(flags, SF_PLAY_INFINITE)) {
            PlayMusicStream(rpstate->musics[id]);
        } else {
            PlaySound(rpstate->sounds[id]);
        }
    }
}

u32 PlatformSoundCountPlaying(SoundId id) {
    RaylibPlatformState* rpstate = GetRaylibPlatformState();

    u32 count = 0;
    for (i32 i = NULLSOUND + 1; i < ArraySizeOf(rpstate->streams); ++i) {
        if (rpstate->playing[i] && rpstate->streams[i] == id) {
            ++count;
        }
    }
    return count;
}
void PlatformStopSound(SoundId id) {
    RaylibPlatformState* rpstate = GetRaylibPlatformState();

    for (i32 i = NULLSOUND + 1; i < ArraySizeOf(rpstate->streams); ++i) {
        if (rpstate->playing[i] && rpstate->streams[i] == id) {
            rpstate->playing[i] = false;
        }
    }
    StopSound(rpstate->sounds[id]);
    StopMusicStream(rpstate->musics[id]);
}

int main(void) {
    PlatformLog(LLOG_INFO, "Raylib Version %s", RAYLIB_VERSION);

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
