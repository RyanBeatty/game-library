#include <stdio.h>
#include <windows.h>

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_DESTROY: {
            PostQuitMessage(0);
            return 0;
        }
        case WM_KEYDOWN: {
            switch (wParam) {
                default: {
                    printf("Unknown key down %lld\n", wParam);
                }
            }
            return 0;
        }
        // case WM_PAINT: {
        //     PAINTSTRUCT ps;
        //     HDC hdc = BeginPaint(hwnd, &ps);

        //     // All painting occurs here, between BeginPaint and EndPaint.

        //     FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));

        //     EndPaint(hwnd, &ps);
        //     return 0;
        // }
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
