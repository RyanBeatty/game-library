#pragma once

// #define WIN32_LEAN_AND_MEAN

////////////////////////////////////////////////////////////
// Core types.
#include <assert.h>
#include <stdint.h>

#define Assert(expr)        \
    {                       \
        if (!(expr)) {      \
            assert((expr)); \
        }                   \
    }

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float f32;
typedef double f64;

#define LPI ((f32)3.1415926535)

typedef u32 SoundId;

extern const SoundId NULLSOUND;

void InitAudio();
SoundId LoadLSoundTest(const char* filename);
// Can load canonical wav files: http://soundfile.sapp.org/doc/WaveFormat/
SoundId LoadLSound(const char* filename);
void PlayLSound(SoundId id, u32 flags);
