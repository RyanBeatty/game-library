#ifndef UNITY_BUILD
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "raylib.h"
#include "raymath.h"

#endif

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
    const int screenWidth = 800;
    const int screenHeight = 450;

    // InitWindow(screenWidth, screenHeight, "raylib [core] example - basic window");

    // SetTargetFPS(60);  // Set our game to run at 60 frames-per-second

    // // Must call at least once.
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) return hr;

    IXAudio2* m_xaudio2{};

    // Create the main audio interface.
    if (FAILED(XAudio2Create(&m_xaudio2, 0, XAUDIO2_DEFAULT_PROCESSOR))) {
        printf("Failed to create XAudio2 instance\n");
    }

    IXAudio2MasteringVoice* m_pXAudio2MasteringVoice{};
    if (FAILED(m_xaudio2->CreateMasteringVoice(&m_pXAudio2MasteringVoice))) {
        return 1;
    }

    // Define a format.
    WAVEFORMATEX waveFormatEx{};
    waveFormatEx.wFormatTag = WAVE_FORMAT_PCM;
    waveFormatEx.nChannels = 1;  // 1 channel
    waveFormatEx.nSamplesPerSec = SAMPLESPERSEC;
    waveFormatEx.nBlockAlign = waveFormatEx.nChannels * BITSPERSSAMPLE / 8;
    waveFormatEx.nAvgBytesPerSec = waveFormatEx.nSamplesPerSec * waveFormatEx.nBlockAlign;
    waveFormatEx.wBitsPerSample = BITSPERSSAMPLE;
    waveFormatEx.cbSize = 0;

    IXAudio2SourceVoice* m_pXAudio2SourceVoice{};

    std::array<byte, AUDIOBUFFERSIZEINBYTES> m_buffer{};

    // Create a source voice with that format.
    if (FAILED(m_xaudio2->CreateSourceVoice(&m_pXAudio2SourceVoice, &waveFormatEx))) {
        return 1;
    }

    // Fill a buffer.
    double phase{};
    uint32_t bufferIndex{};
    while (bufferIndex < AUDIOBUFFERSIZEINBYTES) {
        phase += (2 * PI) / SAMPLESPERCYCLE;
        int16_t sample = (int16_t)(sin(phase) * INT16_MAX * VOLUME);
        m_buffer[bufferIndex++] = (byte)sample;  // Values are little-endian.
        m_buffer[bufferIndex++] = (byte)(sample >> 8);
    }

    XAUDIO2_BUFFER xAudio2Buffer{};
    xAudio2Buffer.Flags = XAUDIO2_END_OF_STREAM;
    xAudio2Buffer.AudioBytes = AUDIOBUFFERSIZEINBYTES;
    xAudio2Buffer.pAudioData = m_buffer.data();
    xAudio2Buffer.PlayBegin = 0;
    xAudio2Buffer.PlayLength = 0;
    xAudio2Buffer.LoopBegin = 0;
    xAudio2Buffer.LoopLength = 0;
    xAudio2Buffer.LoopCount = XAUDIO2_LOOP_INFINITE;

    // Submit the buffer to the source voice, and start the voice.
    if (FAILED(m_pXAudio2SourceVoice->SubmitSourceBuffer(&xAudio2Buffer))) {
        return 1;
    }
    if (FAILED(m_pXAudio2SourceVoice->Start(0))) {
        return 1;
    }

    f32 t = 0.0f;
    while (t < 3) {
        sleep(1);
        t += 1.0f;
    }

    // while (!WindowShouldClose())  // Detect window close button or ESC key
    // {
    //     BeginDrawing();

    //     ClearBackground(RAYWHITE);

    //     DrawText("Congrats! You created your first window!", 190, 200, 20, LIGHTGRAY);

    //     EndDrawing();
    // }

    // CloseWindow();

    return 0;
}
