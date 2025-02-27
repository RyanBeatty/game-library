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

class VoiceCallback : public IXAudio2VoiceCallback {
   public:
    HANDLE hBufferEndEvent;
    VoiceCallback() : hBufferEndEvent(CreateEvent(NULL, FALSE, FALSE, NULL)) {}
    ~VoiceCallback() { CloseHandle(hBufferEndEvent); }

    // Called when the voice has just finished playing a contiguous audio stream.
    void OnStreamEnd() {
        printf("stream ended\n");
        SetEvent(hBufferEndEvent);
    }

    // Unused methods are stubs
    void OnVoiceProcessingPassEnd() {
        //  printf("void processing pass end\n");
    }
    void OnVoiceProcessingPassStart(UINT32 SamplesRequired) {}
    void OnBufferEnd(void* pBufferContext) { printf("buffer ended\n"); }
    void OnBufferStart(void* pBufferContext) {}
    void OnLoopEnd(void* pBufferContext) { printf("loop end\n"); }
    void OnVoiceError(void* pBufferContext, HRESULT Error) {}
};

int main(void) {
    static XAUDIO2_BUFFER XBUFFERS[2] = {0};

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
    WAVEFORMATEX wfx = {0};
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = 1;  // 1 channel
    wfx.nSamplesPerSec = SAMPLESPERSEC;
    wfx.nBlockAlign = wfx.nChannels * BITSPERSSAMPLE / 8;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
    wfx.wBitsPerSample = BITSPERSSAMPLE;
    wfx.cbSize = 0;

    IXAudio2SourceVoice* source_voice = {0};

    // Create a source voice with that format.
    if (FAILED(xaudio2->CreateSourceVoice(&source_voice, &wfx))) {
        return 1;
    }

    VoiceCallback voice_callback;
    if (FAILED(hr = xaudio2->CreateSourceVoice(&source_voice, (WAVEFORMATEX*)&wfx, 0, XAUDIO2_DEFAULT_FREQ_RATIO,
                                               &voice_callback, NULL, NULL))) {
        return 1;
    }

    // This buffer needs to live a long life. Needs to be allocated until any sound
    // data is not reading from it.
    u8 buffer[AUDIOBUFFERSIZEINBYTES] = {0};
    u32 bsize = 0;

    // Fill a buffer.
    double phase = 0.0f;
    while (bsize < AUDIOBUFFERSIZEINBYTES) {
        phase += (2 * PI) / SAMPLESPERCYCLE;
        int16_t sample = (int16_t)(sin(phase) * INT16_MAX * VOLUME);
        buffer[bsize++] = (byte)sample;  // Values are little-endian.
        buffer[bsize++] = (byte)(sample >> 8);
    }

    {
        XAUDIO2_BUFFER* xbuffer = &XBUFFERS[0];
        xbuffer->Flags = XAUDIO2_END_OF_STREAM;  // TODO: maybe remove.
        xbuffer->AudioBytes = AUDIOBUFFERSIZEINBYTES;
        xbuffer->pAudioData = buffer;
        xbuffer->PlayBegin = 0;
        xbuffer->PlayLength = 0;
        xbuffer->LoopBegin = 0;
        xbuffer->LoopLength = 0;
        xbuffer->LoopCount = XAUDIO2_LOOP_INFINITE;
    }

    // Submit the buffer to the source voice, and start the voice.
    if (FAILED(source_voice->SubmitSourceBuffer(&XBUFFERS[0]))) {
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
