#include <stdbool.h>
#include <stdio.h>
#include <windows.h>
// Include after windows.h

#ifndef UNITY_BUILD
#include "base.h"
#include "glad/glad.h"
#include "platform.h"
#endif

#define MAX_KEYS 256
typedef struct {
    // The polling window is the time between the calls to ProcessInput().

    // Did the key end the polling window down.
    bool down[MAX_KEYS];
    // How many half transitions did the key go though during the polling window.
    u32 half_transitions[MAX_KEYS];
} Win32Input;

typedef struct {
    // rectangle of the drawable area of the window. this is different from the full window size.
    RECT client_rect;

    f32 target_fps;

    HINSTANCE hinstance;
    HDC dc;
    HWND window_handle;
    int n_cmd_show;

    Win32Input inputs[2];
    Win32Input* cur_input;
    Win32Input* prev_input;
} Win32State;

static Win32State W32State = {0};
#define GetWin32State() (&W32State)

////////////////////////////////////////////////////////////////////////////////////
// Input

// TODO: These methods don't handle the case where you are mashing and managed to do a full transition within a frame.
LCORE bool PlatformKeyPressed(LKey key) {
    return GetWin32State()->cur_input->down[key] && !GetWin32State()->prev_input->down[key];
}
LCORE bool PlatformKeyReleased(LKey key) {
    return !GetWin32State()->cur_input->down[key] && GetWin32State()->prev_input->down[key];
}
LCORE bool PlatformKeyDown(LKey key) { return GetWin32State()->cur_input->down[key]; }

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_DESTROY: {
            PostQuitMessage(0);
            return 0;
        }
        case WM_KEYDOWN:
        case WM_KEYUP: {
            PlatformLog(LLOG_ERROR, "windows proc message for key up/down should be hanlded by ProcessInput. Key: %d",
                        wParam);
            Assert(false);
            return 0;
        }
        default: {
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
        }
    }
}

HWND Win32CreateWindow(HINSTANCE hinstance, u32 width, u32 height, HDC* dc) {
    static char CLASS_NAME[] = "L Engine";

    WNDCLASS wc = {
        .lpfnWndProc = WindowProc,
        .hInstance = hinstance,
        .lpszClassName = CLASS_NAME,
        .style = CS_OWNDC,  // I think important for opengl.
    };

    ATOM res = RegisterClass(&wc);
    if (res == 0) {
        DWORD err = GetLastError();
        fprintf(stderr, "Error registering class %ld", err);
        exit(1);
    }

    int wstyle = WS_OVERLAPPEDWINDOW;
    int wstyleex = 0;

    // Create window takes the full window dims as parameters this includes space for toolbar and borders.
    // We want to translate our desired width/height which is for the drawing area to window width/height which
    // we can do via AdjustWindowRectEx.
    RECT rect = {0, 0, width, height};
    AdjustWindowRectEx(&rect, wstyle, FALSE, wstyleex);
    int actual_width = rect.right - rect.left;
    int actual_height = rect.bottom - rect.top;

    HWND window_handle = CreateWindowEx(wstyleex,    // Optional window styles.
                                        CLASS_NAME,  // Window class
                                        "L Engine",  // Window text
                                        wstyle,      // dwStyle Window style

                                        // pos
                                        CW_USEDEFAULT,  // X
                                        CW_USEDEFAULT,  // Y
                                        // size
                                        actual_width,   // width
                                        actual_height,  // height

                                        NULL,       // Parent window
                                        NULL,       // Menu
                                        hinstance,  // Instance handle
                                        NULL        // Additional application data
    );

    if (window_handle == NULL) {
        fprintf(stderr, "Failed to create window");
        exit(1);
    }

    *dc = GetDC(window_handle);
    return window_handle;
}

// Define some constants and gl functions to avoid dependency on glew
#define WGL_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB 0x2092
#define WGL_CONTEXT_PROFILE_MASK_ARB 0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB 0x00000001

typedef HGLRC(WINAPI* PFNWGLCREATECONTEXTATTRIBSARBPROC)(HDC, HGLRC, const int*);
PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = NULL;

typedef BOOL(WINAPI* WGLSWAPINTERVALEXT)(int);
WGLSWAPINTERVALEXT wglSwapIntervalEXT = NULL;

void LoadWGLExtensionFunctions() {
    // Load function pointer for func that allows creation of modern gl context.
    wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
    Assert(wglCreateContextAttribsARB);

    wglSwapIntervalEXT = (WGLSWAPINTERVALEXT)wglGetProcAddress("wglSwapIntervalEXT");
    Assert(wglSwapIntervalEXT);
}

// https://www.khronos.org/opengl/wiki/Creating_an_OpenGL_Context_(WGL)
HGLRC Win32CreateOpengGLCtx(HDC dc) {
    PIXELFORMATDESCRIPTOR pfd = {sizeof(PIXELFORMATDESCRIPTOR),
                                 1,
                                 PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,  // Flags
                                 PFD_TYPE_RGBA,  // The kind of framebuffer. RGBA or palette.
                                 32,             // Colordepth of the framebuffer.
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 24,  // Number of bits for the depthbuffer
                                 8,   // Number of bits for the stencilbuffer
                                 0,   // Number of Aux buffers in the framebuffer.
                                 PFD_MAIN_PLANE,
                                 0,
                                 0,
                                 0,
                                 0};

    i32 w32_chosen_px_format = ChoosePixelFormat(dc, &pfd);
    Assert(w32_chosen_px_format != 0);
    SetPixelFormat(dc, w32_chosen_px_format, &pfd);

    // Create temp ctx in order to load GLAD extension for easy gl function loading (I think required for
    // access to gl functions > 1.0).
    {
        // Create temp ctx so we can load glad.
        HGLRC temp_glctx = wglCreateContext(dc);
        Assert(temp_glctx != NULL);
        wglMakeCurrent(dc, temp_glctx);

        LoadWGLExtensionFunctions();

        // Create modern gl context for gl 3.3.0
        i32 attribs[] = {
            WGL_CONTEXT_MAJOR_VERSION_ARB,    3, WGL_CONTEXT_MINOR_VERSION_ARB, 3, WGL_CONTEXT_PROFILE_MASK_ARB,
            WGL_CONTEXT_CORE_PROFILE_BIT_ARB, 0};
        HGLRC glctx = wglCreateContextAttribsARB(dc, 0, attribs);
        Assert(glctx != NULL);

        // Make our final context current.
        wglDeleteContext(temp_glctx);
        wglMakeCurrent(dc, glctx);

        // Load glad. After this point we can use any gl functions.
        if (!gladLoadGL()) {
            fprintf(stderr, "Failed to load GLAD\n");
            exit(1);
        }

        // Display current version of opengl.
        printf("OpenGL Version: %s\n", glGetString(GL_VERSION));

        return glctx;
    }
}

///////////////////////////////////////////////////////////////////////////////////////
// Audio

#include <math.h>
#include <unistd.h>
#include <windows.h>
#include <xaudio2.h>

#define AUDIO_NUM_CHANNELS 1
#define SAMPLESPERSEC 44100  // 44,100 samples per second.
#define BITSPERSSAMPLE 16    // 16 bits per sample.

#define AUDIO_BLOCK_ALIGN (AUDIO_NUM_CHANNELS * (BITSPERSSAMPLE / 8))
#define AUDIO_BYTE_RATE (SAMPLESPERSEC * AUDIO_NUM_CHANNELS * (BITSPERSSAMPLE / 8))

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

void PlatformInitAudio(byte* buffer, isize cap) {
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
SoundId PlatformLoadSound(const char* filename) {
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
        sound->data = New(&AUDIO.soundmem, sound->cap, byte);

        byte* subchunk2_end = cur + subchunk2_size;
        while (cur < subchunk2_end) {
            *PushS(sound) = *cur++;
        }
    }

    return id;
}

void PlatformPlaySoundLimit(SoundId id, SoundFlag flags, u32 limit) {
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

u32 PlatformSoundCountPlaying(SoundId id) {
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
void PlatformStopSound(SoundId id) {
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

///////////////////////////////////////////////////////////////////////////////////////
LCORE void PlatformLog(LogLevel level, const char* fmt, ...) {
    static char LEVELS[][16] = {
        "NONE", "DEBUG", "INFO", "WARN", "ERROR", "COUNT",
    };
    char buffer[256] = {0};

    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    printf("%s: %s\n", LEVELS[level], buffer);
}

// TODO: Do we really need this? maybe can remove.
u32 PlatformScreenWidth() {
    Win32State* w32state = GetWin32State();
    return (u32)(w32state->client_rect.right - w32state->client_rect.left);
}
u32 PlatformScreenHeight() {
    Win32State* w32state = GetWin32State();
    return (u32)(w32state->client_rect.bottom - w32state->client_rect.top);
}

LCORE void PlatformInit(u32 width, u32 height, u32 target_fps) {
    Win32State* w32state = GetWin32State();

    w32state->cur_input = &w32state->inputs[1];
    w32state->prev_input = &w32state->inputs[0];

    w32state->target_fps = target_fps;

    // Needed to get correct sized window after creating window.
    // https://stackoverflow.com/questions/29195137/createwindow-on-windows-8-seems-to-create-window-too-large
    SetProcessDPIAware();
    w32state->window_handle = Win32CreateWindow(w32state->hinstance, width, height, &w32state->dc);
    {
        Win32CreateOpengGLCtx(w32state->dc);

        // Set Vsync based on target FPS.
        {
            // Query the monitor refresh rate.
            DEVMODE dm = {};
            dm.dmSize = sizeof(dm);
            if (EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dm)) {
                PlatformLog(LLOG_INFO, "Refresh rate: %d Hz\n", dm.dmDisplayFrequency);
            }

            // Require the display fps to be some multiple of what we want to render at.
            // TODO: Make this more robust in the future.
            u32 display_fps = dm.dmDisplayFrequency;
            Assert(target_fps <= display_fps && display_fps % target_fps == 0);

            // Sync buffer flip on vsync
            // https://www.khronos.org/opengl/wiki/Swap_Interval
            u32 swap_interval = display_fps / target_fps;
            Assert(wglSwapIntervalEXT(swap_interval));
        }
    }

    if (!GetClientRect(w32state->window_handle, &w32state->client_rect)) {
        PlatformLog(LLOG_ERROR, "Failed to get window client rect");
        exit(1);
    }

    // TODO: Respond to window resize events.
    // TODO: Move this into game renderer. should just always call this.
    // glViewport(0, 0, PlatformScreenWidth(), PlatformScreenHeight());

    ShowWindow(w32state->window_handle, w32state->n_cmd_show);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    Win32State* w32state = GetWin32State();
    w32state->hinstance = hInstance;
    w32state->n_cmd_show = nCmdShow;

    GameInit();

    LARGE_INTEGER ticks_per_sec = {0};
    QueryPerformanceFrequency(&ticks_per_sec);
    f64 sec_per_ticks = 1.0f / ((f64)ticks_per_sec.QuadPart);

    LARGE_INTEGER ticks_last = {0};
    QueryPerformanceCounter(&ticks_last);

    LARGE_INTEGER ticks = {0};

    Win32Input zero_input = {0};

    bool running = true;
    MSG msg = {};
    // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getmessage
    while (running) {
        // Respond to input
        {
            // Swap prev and cur input.
            Win32Input* temp = w32state->prev_input;
            w32state->prev_input = w32state->cur_input;
            w32state->cur_input = temp;
            // Zero out cur input and set down state from the prev input state.
            Win32Input* cur_input = w32state->cur_input;
            *cur_input = zero_input;
            for (i32 i = 0; i < ArraySizeOf(cur_input->down); ++i) {
                cur_input->down[i] = w32state->prev_input->down[i];
            }

            while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
                if (msg.message == WM_QUIT) {
                    running = false;
                    break;
                } else if (msg.message == WM_KEYDOWN || msg.message == WM_KEYUP) {
                    LKey key = (LKey)msg.wParam;
                    bool down = msg.message == WM_KEYDOWN;
                    if (down != cur_input->down[key]) {
                        ++cur_input->half_transitions[key];
                    }
                    cur_input->down[key] = down;

                    // PlatformLog(LLOG_DEBUG, "key event: %c down: %d half_transitions: %d", key, cur_input->down[key],
                    //             cur_input->half_transitions[key]);

                    // TODO: Expose platform function for quitting and move this to game layer.
                    if (cur_input->down[LKEY_ESCAPE]) {
                        PostQuitMessage(0);
                    }
                } else {
                    // Translates char input related messages (e.g. key up/down). seems like it doesn't modify
                    // msg struct and just adds more messages to the queue. do we even need this?
                    // TranslateMessage(&msg);
                    // Calls our windows procedure to process the message.
                    DispatchMessage(&msg);
                }
            }
        }

        QueryPerformanceCounter(&ticks);
        // i64 ticks_dt = ticks.QuadPart - ticks_last.QuadPart;
        f64 dt = (ticks.QuadPart - ticks_last.QuadPart) * sec_per_ticks;
        ticks_last = ticks;

        // // This is a filthy hack, but we wait until dt is roughly 1/60th of a second to have a fixed
        // // timestep. I should really fix this by decoupling the update from render step and just update
        // // at fixed time intervals instead of having them be lock step like they are currently.
        // // https://medium.com/%40tglaiel/how-to-make-your-game-run-at-60fps-24c61210fe75
        // //
        // https://code.tutsplus.com/how-to-create-a-custom-2d-physics-engine-the-core-engine--gamedev-7493t#timestepping
        // while (dt < 1 / 60.0f) {
        //     QueryPerformanceCounter(&ticks);
        //     dt += (ticks.QuadPart - ticks_last.QuadPart) * sec_per_ticks;
        //     ticks_last = ticks;
        // }

        // printf("dt: %f\n", dt);

        {
            // glClearColor(0.2f, 0.3f, 0.4f, 1.0f);
            // glClear(GL_COLOR_BUFFER_BIT);
            GameTick(dt);

            SwapBuffers(w32state->dc);
            // Sync/blocking
            // glFinish();
        }
    }

    return 0;
}
