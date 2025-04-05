
// #define WIN32_LEAN_AND_MEAN
#include "audio.h"

#include <math.h>
#include <unistd.h>
#include <windows.h>
#include <xaudio2.h>

#define AUDIO_NUM_CHANNELS 1
#define SAMPLESPERSEC 44100  // 44,100 samples per second.
#define BITSPERSSAMPLE 16    // 16 bits per sample.

#define AUDIO_BLOCK_ALIGN (AUDIO_NUM_CHANNELS * (BITSPERSSAMPLE / 8))
#define AUDIO_BYTE_RATE (SAMPLESPERSEC * AUDIO_NUM_CHANNELS * (BITSPERSSAMPLE / 8))

#define MAX_SOUND_SIZE (1000 * 1000)

const SoundId NULLSOUND = 0;

typedef struct {
    u32 len;
    u32 cap;
    u8 buffer[MAX_SOUND_SIZE];
} LSound;

typedef struct {
    IXAudio2SourceVoice* voice;
    XAUDIO2_BUFFER xbuffer;
    bool free;
} AudioStream;

void WINAPI OnBufferEnd(IXAudio2VoiceCallback* this, void* ctx) {
    AudioStream* stream = (AudioStream*)ctx;
    stream->free = true;
}

void WINAPI OnStreamEnd(IXAudio2VoiceCallback* this) {}
void WINAPI OnVoiceProcessingPassEnd(IXAudio2VoiceCallback* this) {}
void WINAPI OnVoiceProcessingPassStart(IXAudio2VoiceCallback* this, UINT32 samples_required) {}
void WINAPI OnBufferStart(IXAudio2VoiceCallback* this, void* ctx) {}
void WINAPI OnLoopEnd(IXAudio2VoiceCallback* this, void* ctx) {}
void WINAPI OnVoiceError(IXAudio2VoiceCallback* this, void* ctx, HRESULT error) {}

#define NUM_STREAMS 10

#define MAX_SOUNDS 10

typedef struct {
    IXAudio2* xaudio;
    IXAudio2MasteringVoice* xaudioMasterVoice;
    IXAudio2VoiceCallback callbacks;

    u32 cap;
    AudioStream streams[NUM_STREAMS];

    u32 soundlen;
    u32 soundcap;
    LSound sounds[MAX_SOUNDS];
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

void InitAudio() {
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
    wfx.nChannels = AUDIO_NUM_CHANNELS;
    wfx.nSamplesPerSec = SAMPLESPERSEC;
    wfx.wBitsPerSample = BITSPERSSAMPLE;
    wfx.nBlockAlign = AUDIO_BLOCK_ALIGN;
    wfx.nAvgBytesPerSec = AUDIO_BYTE_RATE;
    wfx.cbSize = 0;

    AUDIO.cap = NUM_STREAMS;
    for (u32 i = 0; i < AUDIO.cap; ++i) {
        AudioStream* stream = &AUDIO.streams[i];
        stream->xbuffer.Flags = XAUDIO2_END_OF_STREAM;
        stream->xbuffer.pContext = &AUDIO.streams[i];
        stream->free = true;

        res = XAUDIO2()->CreateSourceVoice(AUDIO.xaudio, &stream->voice, &wfx, 0, XAUDIO2_DEFAULT_FREQ_RATIO,
                                           &AUDIO.callbacks, NULL, NULL);

        if (FAILED(res)) {
            fprintf(stderr, "Failed CreateSourceVoice\n");
            exit(1);
        }
    }

    ClearLSounds();
}

void ClearLSounds() {
    memset(AUDIO.sounds, 0, sizeof(AUDIO.sounds));
    // sound 0 is blank sound sentinal.
    AUDIO.soundlen = 1;
    AUDIO.soundcap = MAX_SOUNDS;
    for (u32 i = 0; i < AUDIO.soundcap; ++i) {
        AUDIO.sounds[i].cap = MAX_SOUND_SIZE;
    }
}

SoundId LoadLSoundTest(const char* filename) {
    f32 VOLUME = 0.5;                                        // 50% volume.
    u32 CYCLESPERSEC = 220;                                  // 220 cycles per second (frequency of the audible tone).
    u32 SAMPLESPERCYCLE = ((SAMPLESPERSEC / CYCLESPERSEC));  // 200 samples per cycle.
    u32 AUDIOBUFFERSIZEINCYCLES = 10;                        // 10 cycles per audio buffer.
    u32 AUDIOBUFFERSIZEINSAMPLES = (SAMPLESPERCYCLE * AUDIOBUFFERSIZEINCYCLES);      // 2,000 samples per buffer.
    u32 AUDIOBUFFERSIZEINBYTES = (AUDIOBUFFERSIZEINSAMPLES * (BITSPERSSAMPLE / 8));  // 4,000 bytes per buffer.

    Assert(AUDIO.soundlen < AUDIO.soundcap);
    SoundId id = AUDIO.soundlen++;
    LSound* sound = &AUDIO.sounds[id];

    // Fill a buffer.
    double phase = 0.0f;
    while (sound->len < AUDIOBUFFERSIZEINBYTES) {
        phase += (2 * LPI) / SAMPLESPERCYCLE;
        int16_t sample = (int16_t)(sin(phase) * INT16_MAX * VOLUME);
        sound->buffer[sound->len++] = (byte)sample;  // Values are little-endian.
        sound->buffer[sound->len++] = (byte)(sample >> 8);
    }
    Assert(sound->len <= sound->cap);
    return id;
}

// Can load canonical wav files: http://soundfile.sapp.org/doc/WaveFormat/
SoundId LoadLSound(const char* filename) {
    Assert(AUDIO.soundlen < AUDIO.soundcap);
    SoundId id = AUDIO.soundlen++;
    LSound* sound = &AUDIO.sounds[id];

    u32 bsize = 0;
    u32 bcap = MAX_SOUND_SIZE;
    u8 buffer[MAX_SOUND_SIZE] = {0};

    FILE* file = fopen(filename, "rb");  // Open file in binary mode
    Assert(file);

    // Seek to the end to determine the file size
    fseek(file, 0, SEEK_END);
    bsize = ftell(file);
    Assert(bsize <= bcap);
    rewind(file);

    fread(buffer, 1, bsize, file);
    fclose(file);

    u8* cur = buffer;
    u8* end = buffer + bsize;

    Assert(end - cur > 4);
    Assert(*cur++ == 'R' && *cur++ == 'I' && *cur++ == 'F' && *cur++ == 'F');

    // NOTE: Assumes little endian.
    Assert(end - cur > 4);
    u32 chunk_size = 0;
    chunk_size |= ((u32)*cur++) << 0;
    chunk_size |= ((u32)*cur++) << 8;
    chunk_size |= ((u32)*cur++) << 16;
    chunk_size |= ((u32)*cur++) << 24;

    Assert(end - cur > 4);
    Assert(*cur++ == 'W' && *cur++ == 'A' && *cur++ == 'V' && *cur++ == 'E');

    Assert(end - cur > 4);
    Assert(*cur++ == 'f' && *cur++ == 'm' && *cur++ == 't' && *cur++ == ' ');

    Assert(end - cur > 4);
    u32 subchunk_size = 0;
    subchunk_size |= ((u32)*cur++) << 0;
    subchunk_size |= ((u32)*cur++) << 8;
    subchunk_size |= ((u32)*cur++) << 16;
    subchunk_size |= ((u32)*cur++) << 24;
    Assert(subchunk_size == 16 || subchunk_size == 18);

    Assert(end - cur > 2);
    u16 fmt = 0;
    fmt |= ((u16)*cur++) << 0;
    fmt |= ((u16)*cur++) << 8;
    Assert(fmt == 1);

    Assert(end - cur > 2);
    u16 nchannels = 0;
    nchannels |= ((u16)*cur++) << 0;
    nchannels |= ((u16)*cur++) << 8;
    Assert(nchannels == 1);

    Assert(end - cur > 4);
    u32 sample_rate = 0;
    sample_rate |= ((u32)*cur++) << 0;
    sample_rate |= ((u32)*cur++) << 8;
    sample_rate |= ((u32)*cur++) << 16;
    sample_rate |= ((u32)*cur++) << 24;
    Assert(sample_rate == SAMPLESPERSEC);

    Assert(end - cur > 4);
    u32 byte_rate = 0;
    byte_rate |= ((u32)*cur++) << 0;
    byte_rate |= ((u32)*cur++) << 8;
    byte_rate |= ((u32)*cur++) << 16;
    byte_rate |= ((u32)*cur++) << 24;
    Assert(byte_rate == AUDIO_BYTE_RATE);

    Assert(end - cur > 2);
    u16 block_align = 0;
    block_align |= ((u16)*cur++) << 0;
    block_align |= ((u16)*cur++) << 8;
    Assert(block_align == AUDIO_BLOCK_ALIGN);

    Assert(end - cur > 2);
    u16 bits_per_sample = 0;
    bits_per_sample |= ((u16)*cur++) << 0;
    bits_per_sample |= ((u16)*cur++) << 8;
    Assert(bits_per_sample == BITSPERSSAMPLE);

    if (subchunk_size == 18) {
        cur += 2;  // Consume extra 2 bytes of extension data
    }

    Assert(end - cur > 4);
    Assert(*cur++ == 'd' && *cur++ == 'a' && *cur++ == 't' && *cur++ == 'a');

    Assert(end - cur > 4);
    u32 subchunk2_size = 0;
    subchunk2_size |= ((u32)*cur++) << 0;
    subchunk2_size |= ((u32)*cur++) << 8;
    subchunk2_size |= ((u32)*cur++) << 16;
    subchunk2_size |= ((u32)*cur++) << 24;
    Assert(subchunk2_size < sound->cap);

    u8* subchunk2_end = cur + subchunk2_size;
    while (cur < subchunk2_end) {
        sound->buffer[sound->len++] = *cur++;
    }

    Assert(sound->len <= sound->cap);
    return id;
}

void PlayLSound(SoundId id, u32 flags) {
    Assert(id < AUDIO.soundlen);
    LSound* sound = &AUDIO.sounds[id];

    for (u32 i = 0; i < AUDIO.cap; ++i) {
        AudioStream* stream = &AUDIO.streams[i];
        if (stream->free) {
            stream->free = false;

            stream->xbuffer.AudioBytes = sound->len;
            stream->xbuffer.pAudioData = sound->buffer;

            stream->voice->lpVtbl->SubmitSourceBuffer(stream->voice, &stream->xbuffer, 0);
            stream->voice->lpVtbl->Start(stream->voice, 0, 0);
            return;
        }
    }
}
