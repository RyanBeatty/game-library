#include <stdbool.h>
#include <stdio.h>
#include <windows.h>
// Include after windows.h

#ifndef UNITY_BUILD
#include "glad/glad.h"
#include "shared.h"
#endif

// https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
typedef enum { WKEY_NONE = 0, WKEY_ESCAPE = 0X1B, WKEY_A = 0x41 } W32Key;

// // https://www.ascii-code.com/
// typedef enum {
//     LKEY_NONE = 0,
//     LKEY_ESCAPE = 33,
//     LKEY_A = 65,
// } LKey;

// #define MAX_KEYS 256

// static LKey W32KeyToLKey(W32Key key) {
//     switch (key) {
//         case WKEY_ESCAPE: {
//             return LKEY_ESCAPE;
//         }
//         case WKEY_A: {
//             return LKEY_A;
//         }
//         default: {
//             return LKEY_NONE;
//         }
//     }
// }

// typedef struct {
//     bool down[MAX_KEYS];
//     bool up[MAX_KEYS];
//     bool was_down[MAX_KEYS];
// } KeyboardInput;

// static KeyboardInput W32Input;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_DESTROY: {
            PostQuitMessage(0);
            return 0;
        }
        // https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-keydown
        case WM_KEYDOWN: {
            // if (key == LKEY_ESCAPE) {
            //     PostQuitMessage(0);
            // }
            switch (wParam) {
                case WKEY_A: {
                    // i32 repeat_count = lParam & 0xFFFF;
                    printf("key a down\n");
                    break;
                }
                case VK_ESCAPE: {
                    PostQuitMessage(0);
                    break;
                }
                default: {
                    printf("Unknown key down %lld\n", wParam);
                }
            }
            return 0;
        }
        // https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-keyup
        case WM_KEYUP: {
            // https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
            switch (wParam) {
                case WKEY_A: {
                    printf("Key a up\n");
                    break;
                }
                default: {
                    printf("Unknown key up %lld\n", wParam);
                }
            }
            return 0;
        }
        default: {
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
        }
    }
}

HWND Win32CreateWindow(HINSTANCE hinstance, HDC* dc) {
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

    HWND window_handle = CreateWindowEx(0,                    // Optional window styles.
                                        CLASS_NAME,           // Window class
                                        "L Engine",           // Window text
                                        WS_OVERLAPPEDWINDOW,  // dwStyle Window style

                                        // pos
                                        CW_USEDEFAULT,  // X
                                        CW_USEDEFAULT,  // Y
                                        // size
                                        CW_USEDEFAULT,  // width
                                        CW_USEDEFAULT,  // height

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

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    HDC dc;
    HWND window_handle = Win32CreateWindow(hInstance, &dc);
    {
        HGLRC glctx = Win32CreateOpengGLCtx(dc);
        // Sync buffer flip on vsync
        // https://www.khronos.org/opengl/wiki/Swap_Interval
        Assert(wglSwapIntervalEXT(1));
    }

    RECT wrect = {0};
    if (!GetWindowRect(window_handle, &wrect)) {
        fprintf(stderr, "Failed to get window rect\n");
        exit(1);
    }

    // TODO: Respond to window resize events.
    glViewport(0, 0, (i32)(wrect.right - wrect.left), (i32)(wrect.bottom - wrect.top));

    ShowWindow(window_handle, nCmdShow);

    LARGE_INTEGER ticks_per_sec = {0};
    QueryPerformanceFrequency(&ticks_per_sec);
    f64 sec_per_ticks = 1.0f / ((f64)ticks_per_sec.QuadPart);

    LARGE_INTEGER ticks_last = {0};
    QueryPerformanceCounter(&ticks_last);

    LARGE_INTEGER ticks = {0};

    bool running = true;
    MSG msg = {};
    // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getmessage
    while (running) {
        // Respond to input
        while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                running = false;
                break;
            }
            // Translates char input related messages (e.g. key up/down). seems like it doesn't modify
            // msg struct and just adds more messages to the queue. do we even need this?
            // TranslateMessage(&msg);
            // Calls our windows procedure to process the message.
            DispatchMessage(&msg);
        }

        QueryPerformanceCounter(&ticks);
        i64 ticks_dt = ticks.QuadPart - ticks_last.QuadPart;
        f64 dt = ticks_dt * sec_per_ticks;
        printf("dt: %f\n", dt);
        ticks_last = ticks;

        {
            glClearColor(0.2f, 0.3f, 0.4f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            SwapBuffers(dc);
            // glFinish();
        }
    }

    return 0;
}
