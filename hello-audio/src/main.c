#ifndef UNITY_BUILD
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "raylib.h"
#include "raymath.h"

#endif

#include <unistd.h>
#include <windows.h>
#include <xaudio2.h>

////////////////////////////////////////////////////////////
// Core types.

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

#define F32_MAX FLT_MAX
// Min value a float can differ by.
#define F32_EPSILON FLT_EPSILON

typedef f32 rad;
typedef f32 deg;

#define LPI ((f32)3.1415926535)
#define LTAU ((f32)6.2831853071)
#define LPHI ((f32)1.6180339887)  // golden ratio

#define Deg2Rad(d) ((d) * LTAU / 360.0f)
#define Rad2Deg(r) ((r) * 360.0f / LTAU)
#define RGB01(v) ((v) / 255.0f)

constexpr WORD BITSPERSSAMPLE = 16;           // 16 bits per sample.
constexpr DWORD SAMPLESPERSEC = 44100;        // 44,100 samples per second.
constexpr double CYCLESPERSEC = 220.0;        // 220 cycles per second (frequency of the audible tone).
constexpr double VOLUME = 0.5;                // 50% volume.
constexpr WORD AUDIOBUFFERSIZEINCYCLES = 10;  // 10 cycles per audio buffer.
constexpr double PI = 3.14159265358979323846;

// Calculated constants.
constexpr DWORD SAMPLESPERCYCLE = (DWORD)(SAMPLESPERSEC / CYCLESPERSEC);                  // 200 samples per cycle.
constexpr DWORD AUDIOBUFFERSIZEINSAMPLES = SAMPLESPERCYCLE * AUDIOBUFFERSIZEINCYCLES;     // 2,000 samples per buffer.
constexpr UINT32 AUDIOBUFFERSIZEINBYTES = AUDIOBUFFERSIZEINSAMPLES * BITSPERSSAMPLE / 8;  // 4,000 bytes per buffer.

int main(void) {
    // // Must call at least once.
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) return hr;

    IXAudio2* xaudio2 = {0};

    // Create the main audio interface.
    if (FAILED(XAudio2Create(&xaudio2, 0, XAUDIO2_DEFAULT_PROCESSOR))) {
        printf("Failed to create XAudio2 instance\n");
    }

    IXAudio2MasteringVoice* m_pXAudio2MasteringVoice{};
    if (FAILED(xaudio2->CreateMasteringVoice(&m_pXAudio2MasteringVoice))) {
        return 1;
    }

    // Define a format.
    WAVEFORMATEX waveformatex = {0};
    waveformatex.wFormatTag = WAVE_FORMAT_PCM;
    waveformatex.nChannels = 1;  // 1 channel
    waveformatex.nSamplesPerSec = SAMPLESPERSEC;
    waveformatex.nBlockAlign = waveformatex.nChannels * BITSPERSSAMPLE / 8;
    waveformatex.nAvgBytesPerSec = waveformatex.nSamplesPerSec * waveformatex.nBlockAlign;
    waveformatex.wBitsPerSample = BITSPERSSAMPLE;
    waveformatex.cbSize = 0;

    IXAudio2SourceVoice* source_voice = {0};

    u8 buffer[AUDIOBUFFERSIZEINBYTES] = {0};
    u32 bsize = 0;

    // Create a source voice with that format.
    if (FAILED(xaudio2->CreateSourceVoice(&source_voice, &waveformatex))) {
        return 1;
    }

    // Fill a buffer.
    double phase = 0.0f;
    while (bsize < AUDIOBUFFERSIZEINBYTES) {
        phase += (2 * PI) / SAMPLESPERCYCLE;
        int16_t sample = (int16_t)(sin(phase) * INT16_MAX * VOLUME);
        buffer[bsize++] = (byte)sample;  // Values are little-endian.
        buffer[bsize++] = (byte)(sample >> 8);
    }

    XAUDIO2_BUFFER xaudio2_buffer{};
    xaudio2_buffer.Flags = XAUDIO2_END_OF_STREAM;
    xaudio2_buffer.AudioBytes = AUDIOBUFFERSIZEINBYTES;
    xaudio2_buffer.pAudioData = buffer;
    xaudio2_buffer.PlayBegin = 0;
    xaudio2_buffer.PlayLength = 0;
    xaudio2_buffer.LoopBegin = 0;
    xaudio2_buffer.LoopLength = 0;
    xaudio2_buffer.LoopCount = XAUDIO2_LOOP_INFINITE;

    // Submit the buffer to the source voice, and start the voice.
    if (FAILED(source_voice->SubmitSourceBuffer(&xaudio2_buffer))) {
        return 1;
    }
    if (FAILED(source_voice->Start(0))) {
        return 1;
    }

    f32 t = 0.0f;
    while (t < 3) {
        sleep(1);
        t += 1.0f;
    }

    return 0;
}
