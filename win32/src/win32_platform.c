#include <stdbool.h>
#include <stdio.h>
#include <windows.h>

typedef enum { WKEY_A = 0x41 } WindowsKey;

typedef enum {
    LKEY_A = 65,
} LKey;

#define MAX_KEYS 256
typedef struct {
    bool down[MAX_KEYS];
    bool up[MAX_KEYS];
    bool was_down[MAX_KEYS];
} KeyboardInput;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_DESTROY: {
            PostQuitMessage(0);
            return 0;
        }
        // https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-keydown
        case WM_KEYDOWN: {
            // https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
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

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    static char CLASS_NAME[] = "L Engine";

    WNDCLASS wc = {
        .lpfnWndProc = WindowProc,
        .hInstance = hInstance,
        .lpszClassName = CLASS_NAME,
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
                                        hInstance,  // Instance handle
                                        NULL        // Additional application data
    );

    if (window_handle == NULL) {
        fprintf(stderr, "Failed to create window");
        exit(1);
    }

    ShowWindow(window_handle, nCmdShow);

    MSG msg = {};
    // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getmessage
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
