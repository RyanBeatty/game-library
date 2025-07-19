#pragma once

#include "shared.h"

typedef u32 SoundId;

extern const SoundId NULLSOUND;

void InitAudio(byte* buffer, isize cap);
void ClearLSounds();
SoundId LoadLSoundTest(const char* filename);
// Can load canonical wav files: http://soundfile.sapp.org/doc/WaveFormat/
SoundId LoadLSound(const char* filename);
void PlayLSound(SoundId id, u32 flags);
