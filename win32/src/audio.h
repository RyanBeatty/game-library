#pragma once

#include "base.h"

typedef u32 SoundId;

extern const SoundId NULLSOUND;

typedef enum {
    SF_NONE = 0,
    SF_PLAY_INFINITE = 1 << 0,
} SoundFlag;

void InitAudio(byte* buffer, isize cap);
void ClearLSounds();
SoundId LoadLSoundTest(const char* filename);
// Can load canonical wav files: http://soundfile.sapp.org/doc/WaveFormat/
SoundId LoadLSound(const char* filename);
void PlayLSound(SoundId id, SoundFlag flags);
void PlayLSoundLimit(SoundId id, u32 limit);
u32 GetLSoundPlaying(SoundId id);
void StopLSound(SoundId);
