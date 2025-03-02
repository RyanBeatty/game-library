#ifndef UNITY_BUILD
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "raylib.h"
#include "raymath.h"

#endif

// #define WIN32_LEAN_AND_MEAN
#include <stdbool.h>
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

#define AUDIO_NUM_CHANNELS 1
#define SAMPLESPERSEC 44100         // 44,100 samples per second.
#define BITSPERSSAMPLE 16           // 16 bits per sample.
#define CYCLESPERSEC 220            // 220 cycles per second (frequency of the audible tone).
#define AUDIOBUFFERSIZEINCYCLES 10  // 10 cycles per audio buffer.

#define VOLUME 0.5  // 50% volume.

#define SAMPLESPERCYCLE ((SAMPLESPERSEC / CYCLESPERSEC))                          // 200 samples per cycle.
#define AUDIOBUFFERSIZEINSAMPLES (SAMPLESPERCYCLE * AUDIOBUFFERSIZEINCYCLES)      // 2,000 samples per buffer.
#define AUDIOBUFFERSIZEINBYTES (AUDIOBUFFERSIZEINSAMPLES * (BITSPERSSAMPLE / 8))  // 4,000 bytes per buffer.

typedef struct {
    IXAudio2SourceVoice* voice;
    XAUDIO2_BUFFER buffer;
    bool free;
} AudioStream;

void WINAPI OnBufferEnd(IXAudio2VoiceCallback* this, void* ctx) {
    // AudioStream* stream = (AudioStream*)ctx;
    // stream->inUse = false;
}

void WINAPI OnStreamEnd(IXAudio2VoiceCallback* this) {}
void WINAPI OnVoiceProcessingPassEnd(IXAudio2VoiceCallback* this) {}
void WINAPI OnVoiceProcessingPassStart(IXAudio2VoiceCallback* this, UINT32 samples_required) {}
void WINAPI OnBufferStart(IXAudio2VoiceCallback* this, void* ctx) {}
void WINAPI OnLoopEnd(IXAudio2VoiceCallback* this, void* ctx) {}
void WINAPI OnVoiceError(IXAudio2VoiceCallback* this, void* ctx, HRESULT error) {}

#define NUM_STREAMS 10

typedef struct {
    IXAudio2* xaudio;
    IXAudio2MasteringVoice* xaudioMasterVoice;
    IXAudio2VoiceCallback callbacks;

    u32 cap;
    AudioStream streams[AUDIO_NUM_CHANNELS];
} AudioState;

static AudioState AUDIO = {
    .callbacks = {.lpVtbl = &(IXAudio2VoiceCallbackVtbl){.OnStreamEnd = OnStreamEnd,
                                                         .OnVoiceProcessingPassEnd = OnVoiceProcessingPassEnd,
                                                         .OnVoiceProcessingPassStart = OnVoiceProcessingPassStart,
                                                         .OnBufferEnd = OnBufferEnd,
                                                         .OnBufferStart = OnBufferStart,
                                                         .OnLoopEnd = OnLoopEnd,
                                                         .OnVoiceError = OnVoiceError}}};

#define XAUDIO2() (AUDIO.xaudio->lpVtbl)
#define SVOICE(i) (AUDIO.streams[(i)].voice->lpVtbl)

static void InitAudio() {
    HRESULT res = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(res)) {
        fprintf(stderr, "failed CoInitializeEx\n");
        exit(1);
    }

    res = XAudio2Create(&AUDIO.xaudio, 0, XAUDIO2_DEFAULT_PROCESSOR);
    if (FAILED(res)) {
    }

    res = XAUDIO2()->CreateMasteringVoice(AUDIO.xaudio, &AUDIO.xaudioMasterVoice, AUDIO_NUM_CHANNELS, SAMPLESPERSEC, 0,
                                          NULL, NULL, AudioCategory_GameEffects);
    if (FAILED(res)) {
        fprintf(stderr, "failed IXAudio2_CreateMasteringVoice\n");
        exit(1);
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

    AUDIO.cap = NUM_STREAMS;
    for (int32_t i = 0; i < AUDIO.cap; ++i) {
        AudioStream* stream = &AUDIO.streams[i];
        stream->buffer.Flags = XAUDIO2_END_OF_STREAM;
        stream->buffer.pContext = &AUDIO.streams[i];

        res = XAUDIO2()->CreateSourceVoice(AUDIO.xaudio, &stream->voice, &wfx, 0, XAUDIO2_DEFAULT_FREQ_RATIO,
                                           &AUDIO.callbacks, NULL, NULL);

        if (FAILED(res)) {
            fprintf(stderr, "Failed CreateSourceVoice\n");
            exit(1);
        }
    }
}

int main(void) {
    static u8 BUFFERS[1][AUDIOBUFFERSIZEINBYTES] = {0};

    InitAudio();

    {
        u8* buffer = BUFFERS[0];
        u32 bsize = 0;

        // Fill a buffer.
        double phase = 0.0f;
        while (bsize < AUDIOBUFFERSIZEINBYTES) {
            phase += (2 * LPI) / SAMPLESPERCYCLE;
            int16_t sample = (int16_t)(sin(phase) * INT16_MAX * VOLUME);
            buffer[bsize++] = (byte)sample;  // Values are little-endian.
            buffer[bsize++] = (byte)(sample >> 8);
        }
    }

    {
        XAUDIO2_BUFFER xbuffer = {0};
        xbuffer.Flags = XAUDIO2_END_OF_STREAM;  // TODO: maybe remove.
        xbuffer.AudioBytes = AUDIOBUFFERSIZEINBYTES;
        xbuffer.pAudioData = BUFFERS[0];
        xbuffer.PlayBegin = 0;
        xbuffer.PlayLength = 0;
        xbuffer.LoopBegin = 0;
        xbuffer.LoopLength = 0;
        xbuffer.LoopCount = 6;  // XAUDIO2_LOOP_INFINITE;

        SVOICE(0)->SubmitSourceBuffer(AUDIO.streams[0].voice, &xbuffer, NULL);
        SVOICE(0)->Start(AUDIO.streams[0].voice, 0, 0);
        int i = 0;
        while (i < 3) {
            sleep(1);
            i += 1;
        }
        // source_voice->Start(0))) { return 1; }
    }
    return 0;
}
