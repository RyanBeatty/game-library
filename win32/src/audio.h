#pragma once

#include "base.h"

typedef u32 SoundId;

extern const SoundId NULLSOUND;

typedef enum {
    SF_NONE = 0,
    SF_PLAY_INFINITE = 1 << 0,
} SoundFlag;

void PlatformInitAudio(byte* buffer, isize cap);
void PlatformClearSounds();
// Can load canonical wav files: http://soundfile.sapp.org/doc/WaveFormat/
SoundId PlatformLoadSound(const char* filename);
// limit = 0, means no limit
void PlatformPlaySoundLimit(SoundId id, SoundFlag flags, u32 limit);
#define PlayLSound(id, flags) (PlatformPlaySoundLimit((id), (flags), 0))

u32 GetLSoundPlaying(SoundId id);
void StopLSound(SoundId);
