
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

const SoundId NULLSOUND = 0;

typedef struct {
    byte* data;
    isize len;
    isize cap;
} LSound;

typedef struct {
    IXAudio2SourceVoice* voice;
    XAUDIO2_BUFFER xbuffer;
    bool free;
    SoundId id;  // ID of sound being played if stream not free.
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

#define MAX_SOUNDS 20

typedef struct {
    Arena mem;

    // Holds sound buffer memory. Alive for duration of program.
    Arena soundmem;

    IXAudio2* xaudio;
    IXAudio2MasteringVoice* xaudioMasterVoice;
    IXAudio2VoiceCallback callbacks;

    AudioStream streams[NUM_STREAMS];
    isize cap;

    LSound sounds[MAX_SOUNDS];
    isize soundlen;
    isize soundcap;
} AudioState;

// NOTE: Cannot memset thisor vtable gets wiped!!!! Should probably switch to a Zero init struct.
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

void InitAudio(byte* buffer, isize cap) {
    static bool INIT = false;
    Assert(!INIT);
    INIT = true;

    AUDIO.mem = NewArena(buffer, cap);
    AUDIO.soundmem = NewArena(New(&AUDIO.mem, GB(1), byte), GB(1));

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

    // memset(&AUDIO, 0, sizeof(AUDIO));

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

    PlatformClearSounds();
}

void PlatformClearSounds() {
    memset(AUDIO.sounds, 0, sizeof(AUDIO.sounds));
    // sound 0 is blank sound sentinal.
    AUDIO.soundlen = 1;
    AUDIO.soundcap = MAX_SOUNDS;
    // TODO: I should just have a free flag on each sound and set those to false.
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
        sound->data[sound->len++] = (byte)sample;  // Values are little-endian.
        sound->data[sound->len++] = (byte)(sample >> 8);
    }
    Assert(sound->len <= sound->cap);
    return id;
}

// Can load canonical wav files:
// * http://soundfile.sapp.org/doc/WaveFormat/
// * https://web.archive.org/web/20160304204823/http://www-mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/WAVE.html
// Can load:
// * yt2wav wav files.
// * jsfxr wav files (with sample rate 44k and sample size 16 bit).
SoundId LoadLSound(const char* filename) {
    Assert(AUDIO.soundlen < AUDIO.soundcap);
    SoundId id = AUDIO.soundlen++;
    LSound* sound = &AUDIO.sounds[id];

    FILE* file = fopen(filename, "rb");  // Open file in binary mode
    Assert(file);

    // Seek to the end to determine the file size
    fseek(file, 0, SEEK_END);
    isize bcap = ftell(file);
    rewind(file);

    Arena temp = AUDIO.mem;
    byte* buffer = New(&temp, bcap, byte);

    fread(buffer, 1, bcap, file);
    fclose(file);

    byte* cur = buffer;
    byte* end = buffer + bcap;

    // RIFF header
    {
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
    }

    // fmt chunk
    {
        Assert(end - cur > 4);
        Assert(*cur++ == 'f' && *cur++ == 'm' && *cur++ == 't' && *cur++ == ' ');

        Assert(end - cur > 4);
        isize fmt_chunk_size = 0;
        fmt_chunk_size |= ((isize)*cur++) << 0;
        fmt_chunk_size |= ((isize)*cur++) << 8;
        fmt_chunk_size |= ((isize)*cur++) << 16;
        fmt_chunk_size |= ((isize)*cur++) << 24;
        Assert(fmt_chunk_size == 16 || fmt_chunk_size == 18);
        // Get next chunk offset for later.
        byte* next_chunk_start = cur + fmt_chunk_size;

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

        cur = next_chunk_start;
    }

    // optional LIST chunk
    // * https://www.recordingblogs.com/wiki/list-chunk-of-a-wave-file
    byte peek = *cur;
    if (peek == 'L') {
        Assert(*cur++ == 'L');
        Assert(*cur++ == 'I');
        Assert(*cur++ == 'S');
        Assert(*cur++ == 'T');

        isize chunk_size = 0;
        chunk_size |= ((isize)*cur++) << 0;
        chunk_size |= ((isize)*cur++) << 8;
        chunk_size |= ((isize)*cur++) << 16;
        chunk_size |= ((isize)*cur++) << 24;
        byte* next_chunk_start = cur + chunk_size;

        // Assume we can only parse INFO list chunks.
        Assert(*cur++ == 'I');
        Assert(*cur++ == 'N');
        Assert(*cur++ == 'F');
        Assert(*cur++ == 'O');

        cur = next_chunk_start;
    }

    // data chunk
    {
        Assert(cur < end && end - cur > 4);
        Assert(*cur++ == 'd' && *cur++ == 'a' && *cur++ == 't' && *cur++ == 'a');

        Assert(end - cur > 4);
        u32 subchunk2_size = 0;
        subchunk2_size |= ((u32)*cur++) << 0;
        subchunk2_size |= ((u32)*cur++) << 8;
        subchunk2_size |= ((u32)*cur++) << 16;
        subchunk2_size |= ((u32)*cur++) << 24;

        sound->cap = subchunk2_size;
        sound->len = 0;
        sound->data = New(&AUDIO.soundmem, sound->cap, sizeof(byte));

        byte* subchunk2_end = cur + subchunk2_size;
        while (cur < subchunk2_end) {
            *PushS(sound) = *cur++;
        }
    }

    return id;
}

void PlayLSoundLimit(SoundId id, SoundFlag flags, u32 limit) {
    Assert(id < AUDIO.soundlen);
    LSound* sound = &AUDIO.sounds[id];

    AudioStream* choice = NULL;

    u32 playing = 0;
    for (u32 i = 0; i < AUDIO.cap; ++i) {
        AudioStream* stream = &AUDIO.streams[i];
        if (!stream->free) {
            if (stream->id == id) {
                ++playing;
            }
        } else {
            choice = stream;
        }
    }

    if (choice != NULL && (limit == 0 || playing < limit)) {
        AudioStream* stream = choice;
        stream->free = false;
        stream->id = id;

        stream->xbuffer.AudioBytes = sound->len;
        stream->xbuffer.pAudioData = sound->data;
        stream->xbuffer.LoopCount = (flags & SF_PLAY_INFINITE) ? XAUDIO2_LOOP_INFINITE : 0;

        stream->voice->lpVtbl->SubmitSourceBuffer(stream->voice, &stream->xbuffer, 0);
        stream->voice->lpVtbl->Start(stream->voice, 0, 0);
    }
}

u32 GetLSoundPlaying(SoundId id) {
    AudioState* audio = &AUDIO;
    u32 count = 0;
    for (i32 i = 0; i < audio->cap; ++i) {
        AudioStream* stream = &audio->streams[i];
        if (!stream->free && stream->id == id) {
            ++count;
        }
    }

    return count;
}

// Stops *all* running sounds for the given sound id.
void StopLSound(SoundId id) {
    AudioState* audio = &AUDIO;
    for (i32 i = 0; i < audio->cap; ++i) {
        AudioStream* stream = &audio->streams[i];
        if (!stream->free && stream->id == id) {
            stream->voice->lpVtbl->Stop(stream->voice, 0, 0);
            // Ensure we free the stream. Stop() will not cause any callback events to be fired for
            // buffers. Need to call flush buffers insteam of explicitly marking free since if we play the voice
            // again, it will still have leftover sound buffered.
            stream->voice->lpVtbl->FlushSourceBuffers(stream->voice);
        }
    }
}
