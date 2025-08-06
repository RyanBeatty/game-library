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

    // // Create the window.

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
    assert(w32_chosen_px_format != 0);
    SetPixelFormat(dc, w32_chosen_px_format, &pfd);

    // Create temp ctx in order to load GLAD extension for easy gl function loading (I think required for
    // access to gl functions > 1.0).
    {
        HGLRC temp_glctx = wglCreateContext(dc);
        assert(temp_glctx != NULL);

        wglMakeCurrent(dc, temp_glctx);
        // I think this will show the highest opengl version the driver supports. Not necessarily the functions
        // our ctx has access to
        printf("OpenGL Version: %s\n", glGetString(GL_VERSION));

        wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
        assert(wglCreateContextAttribsARB != NULL);

        i32 attribs[] = {
            WGL_CONTEXT_MAJOR_VERSION_ARB,    3, WGL_CONTEXT_MINOR_VERSION_ARB, 3, WGL_CONTEXT_PROFILE_MASK_ARB,
            WGL_CONTEXT_CORE_PROFILE_BIT_ARB, 0};

        HGLRC glctx = wglCreateContextAttribsARB(dc, 0, attribs);
        assert(glctx != NULL);

        return temp_glctx;
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    HDC dc;
    HWND window_handle = Win32CreateWindow(hInstance, &dc);
    HGLRC glctx = Win32CreateOpengGLCtx(dc);

    ShowWindow(window_handle, nCmdShow);

    MSG msg = {};
    // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getmessage
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);

        glClearColor(0.2f, 0.3f, 0.4f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        SwapBuffers(dc);
    }

    return 0;
}
