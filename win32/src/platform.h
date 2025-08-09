#ifndef UNITY_BUILD
#include <shared.h>

#endif

typedef enum {
    LLOG_NONE = 0,
    LLOG_DEBUG,
    LLOG_INFO,
    LLOG_WARN,
    LLOG_ERROR,
    LLOG_COUNT,
} LogLevel;

LCORE void PlatformLog(LogLevel level, const char* fmt, ...);

// This basically follows the window vkey assignments.
// https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
typedef enum {
    LKEY_NONE = 0,
    LKEY_LBUTTON = 0x01,     // Left mouse button
    LKEY_RBUTTON = 0x02,     // Right mouse button
    LKEY_CANCEL = 0x03,      // Control-break processing
    LKEY_MBUTTON = 0x04,     // Middle mouse button
    LKEY_XBUTTON1 = 0x05,    // X1 mouse button
    LKEY_XBUTTON2 = 0x06,    // X2 mouse button
    LKEY_BACK = 0x08,        // Backspace key
    LKEY_TAB = 0x09,         // Tab key
    LKEY_CLEAR = 0x0C,       // Clear key
    LKEY_ENTER = 0x0D,       // Enter key. *changed from RETURN to ENTER
    LKEY_SHIFT = 0x10,       // Shift key
    LKEY_CONTROL = 0x11,     // Ctrl key
    LKEY_MENU = 0x12,        // Alt key
    LKEY_PAUSE = 0x13,       // Pause key
    LKEY_CAPITAL = 0x14,     // Caps lock key
    LKEY_KANA = 0x15,        // IME Kana mode
    LKEY_HANGUL = 0x15,      // IME Hangul mode
    LKEY_IME_ON = 0x16,      // IME On
    LKEY_JUNJA = 0x17,       // IME Junja mode
    LKEY_FINAL = 0x18,       // IME final mode
    LKEY_HANJA = 0x19,       // IME Hanja mode
    LKEY_KANJI = 0x19,       // IME Kanji mode
    LKEY_IME_OFF = 0x1A,     // IME Off
    LKEY_ESCAPE = 0x1B,      // Esc key
    LKEY_CONVERT = 0x1C,     // IME convert
    LKEY_NONCONVERT = 0x1D,  // IME nonconvert
    LKEY_ACCEPT = 0x1E,      // IME accept
    LKEY_MODECHANGE = 0x1F,  // IME mode change request
    LKEY_SPACE = 0x20,       // Spacebar
    LKEY_PRIOR = 0x21,       // Page up
    LKEY_NEXT = 0x22,        // Page down
    LKEY_END = 0x23,         // End
    LKEY_HOME = 0x24,        // Home
    LKEY_LEFT = 0x25,        // Left arrow
    LKEY_UP = 0x26,          // Up arrow
    LKEY_RIGHT = 0x27,       // Right arrow
    LKEY_DOWN = 0x28,        // Down arrow
    LKEY_SELECT = 0x29,      // Select
    LKEY_PRINT = 0x2A,       // Print
    LKEY_EXECUTE = 0x2B,     // Execute
    LKEY_SNAPSHOT = 0x2C,    // Print screen
    LKEY_INSERT = 0x2D,      // Insert
    LKEY_DELETE = 0x2E,      // Delete
    LKEY_HELP = 0x2F,        // Help
    LKEY_0 = 0x30,           // 0 key
    LKEY_1 = 0x31,           // 1 key
    LKEY_2 = 0x32,           // 2 key
    LKEY_3 = 0x33,           // 3 key
    LKEY_4 = 0x34,           // 4 key
    LKEY_5 = 0x35,           // 5 key
    LKEY_6 = 0x36,           // 6 key
    LKEY_7 = 0x37,           // 7 key
    LKEY_8 = 0x38,           // 8 key
    LKEY_9 = 0x39,           // 9 key
    LKEY_A = 0x41,
    LKEY_B = 0x42,
    LKEY_C = 0x43,
    LKEY_D = 0x44,
    LKEY_E = 0x45,
    LKEY_F = 0x46,
    LKEY_G = 0x47,
    LKEY_H = 0x48,
    LKEY_I = 0x49,
    LKEY_J = 0x4A,
    LKEY_K = 0x4B,
    LKEY_L = 0x4C,
    LKEY_M = 0x4D,
    LKEY_N = 0x4E,
    LKEY_O = 0x4F,
    LKEY_P = 0x50,
    LKEY_Q = 0x51,
    LKEY_R = 0x52,
    LKEY_S = 0x53,
    LKEY_T = 0x54,
    LKEY_U = 0x55,
    LKEY_V = 0x56,
    LKEY_W = 0x57,
    LKEY_X = 0x58,
    LKEY_Y = 0x59,
    LKEY_Z = 0x5A,
    LKEY_LWIN = 0x5B,
    LKEY_RWIN = 0x5C,
    LKEY_APPS = 0x5D,
    LKEY_SLEEP = 0x5F,
    LKEY_NUMPAD0 = 0x60,
    LKEY_NUMPAD1 = 0x61,
    LKEY_NUMPAD2 = 0x62,
    LKEY_NUMPAD3 = 0x63,
    LKEY_NUMPAD4 = 0x64,
    LKEY_NUMPAD5 = 0x65,
    LKEY_NUMPAD6 = 0x66,
    LKEY_NUMPAD7 = 0x67,
    LKEY_NUMPAD8 = 0x68,
    LKEY_NUMPAD9 = 0x69,
    LKEY_MULTIPLY = 0x6A,
    LKEY_ADD = 0x6B,
    LKEY_SEPARATOR = 0x6C,
    LKEY_SUBTRACT = 0x6D,
    LKEY_DECIMAL = 0x6E,
    LKEY_DIVIDE = 0x6F,
    LKEY_F1 = 0x70,
    LKEY_F2 = 0x71,
    LKEY_F3 = 0x72,
    LKEY_F4 = 0x73,
    LKEY_F5 = 0x74,
    LKEY_F6 = 0x75,
    LKEY_F7 = 0x76,
    LKEY_F8 = 0x77,
    LKEY_F9 = 0x78,
    LKEY_F10 = 0x79,
    LKEY_F11 = 0x7A,
    LKEY_F12 = 0x7B,
    LKEY_F13 = 0x7C,
    LKEY_F14 = 0x7D,
    LKEY_F15 = 0x7E,
    LKEY_F16 = 0x7F,
    LKEY_F17 = 0x80,
    LKEY_F18 = 0x81,
    LKEY_F19 = 0x82,
    LKEY_F20 = 0x83,
    LKEY_F21 = 0x84,
    LKEY_F22 = 0x85,
    LKEY_F23 = 0x86,
    LKEY_F24 = 0x87,
    LKEY_NUMLOCK = 0x90,
    LKEY_SCROLL = 0x91,
    LKEY_LSHIFT = 0xA0,
    LKEY_RSHIFT = 0xA1,
    LKEY_LCONTROL = 0xA2,  // left control
    LKEY_RCONTROL = 0xA3,  // right control
    LKEY_LMENU = 0xA4,
    LKEY_RMENU = 0xA5,
    LKEY_BROWSER_BACK = 0xA6,
    LKEY_BROWSER_FORWARD = 0xA7,
    LKEY_BROWSER_REFRESH = 0xA8,
    LKEY_BROWSER_STOP = 0xA9,
    LKEY_BROWSER_SEARCH = 0xAA,
    LKEY_BROWSER_FAVORITES = 0xAB,
    LKEY_BROWSER_HOME = 0xAC,
    LKEY_VOLUME_MUTE = 0xAD,
    LKEY_VOLUME_DOWN = 0xAE,
    LKEY_VOLUME_UP = 0xAF,
    LKEY_MEDIA_NEXT_TRACK = 0xB0,
    LKEY_MEDIA_PREV_TRACK = 0xB1,
    LKEY_MEDIA_STOP = 0xB2,
    LKEY_MEDIA_PLAY_PAUSE = 0xB3,
    LKEY_LAUNCH_MAIL = 0xB4,
    LKEY_LAUNCH_MEDIA_SELECT = 0xB5,
    LKEY_LAUNCH_APP1 = 0xB6,
    LKEY_LAUNCH_APP2 = 0xB7,
    LKEY_OEM_1 = 0xBA,
    LKEY_OEM_PLUS = 0xBB,
    LKEY_OEM_COMMA = 0xBC,
    LKEY_OEM_MINUS = 0xBD,
    LKEY_OEM_PERIOD = 0xBE,
    LKEY_OEM_2 = 0xBF,
    LKEY_OEM_3 = 0xC0,
    LKEY_OEM_4 = 0xDB,
    LKEY_OEM_5 = 0xDC,
    LKEY_OEM_6 = 0xDD,
    LKEY_OEM_7 = 0xDE,
    LKEY_OEM_8 = 0xDF,
    LKEY_OEM_102 = 0xE2,
    LKEY_PROCESSKEY = 0xE5,
    LKEY_PACKET = 0xE7,
    LKEY_ATTN = 0xF6,
    LKEY_CRSEL = 0xF7,
    LKEY_EXSEL = 0xF8,
    LKEY_EREOF = 0xF9,
    LKEY_PLAY = 0xFA,
    LKEY_ZOOM = 0xFB,
    LKEY_NONAME = 0xFC,
    LKEY_PA1 = 0xFD,
    LKEY_OEM_CLEAR = 0xFE
} LKey;

LCORE i32 PlatformKeyToRaylibKey(LKey key);

LCORE bool PlatformKeyPressed(LKey key);
LCORE bool PlatformKeyReleased(LKey key);
LCORE bool PlatformKeyDown(LKey key);

LCORE void PlatformInit(u32 width, u32 height, u32 target_fps);

u32 PlatformScreenWidth();
u32 PlatformScreenHeight();