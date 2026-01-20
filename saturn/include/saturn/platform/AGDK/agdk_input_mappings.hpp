#pragma once

namespace saturn
{
namespace platform
{
namespace agdk
{

enum class AGDKInputAction : uint32_t
{
    release = 0, // Key down event
    press = 1,   // Key up event
    repeat = 2,  // Key repeat event
};

/**
 * @brief The `KeyboardKey` enum class represents the keyboard keys supported by Android.
 *
 * The values of this enum class are based on Android's KeyEvent key codes.
 *
 * @see https://developer.android.com/reference/android/view/KeyEvent
 */
enum class AGDKKeyboardKey : uint32_t
{
    key_a = 29,              // KEYCODE_A
    key_b = 30,              // KEYCODE_B
    key_c = 31,              // KEYCODE_C
    key_d = 32,              // KEYCODE_D
    key_e = 33,              // KEYCODE_E
    key_f = 34,              // KEYCODE_F
    key_g = 35,              // KEYCODE_G
    key_h = 36,              // KEYCODE_H
    key_i = 37,              // KEYCODE_I
    key_j = 38,              // KEYCODE_J
    key_k = 39,              // KEYCODE_K
    key_l = 40,              // KEYCODE_L
    key_m = 41,              // KEYCODE_M
    key_n = 42,              // KEYCODE_N
    key_o = 43,              // KEYCODE_O
    key_p = 44,              // KEYCODE_P
    key_q = 45,              // KEYCODE_Q
    key_r = 46,              // KEYCODE_R
    key_s = 47,              // KEYCODE_S
    key_t = 48,              // KEYCODE_T
    key_u = 49,              // KEYCODE_U
    key_v = 50,              // KEYCODE_V
    key_w = 51,              // KEYCODE_W
    key_x = 52,              // KEYCODE_X
    key_y = 53,              // KEYCODE_Y
    key_z = 54,              // KEYCODE_Z
    key_0 = 7,               // KEYCODE_0
    key_1 = 8,               // KEYCODE_1
    key_2 = 9,               // KEYCODE_2
    key_3 = 10,              // KEYCODE_3
    key_4 = 11,              // KEYCODE_4
    key_5 = 12,              // KEYCODE_5
    key_6 = 13,              // KEYCODE_6
    key_7 = 14,              // KEYCODE_7
    key_8 = 15,              // KEYCODE_8
    key_9 = 16,              // KEYCODE_9
    key_space = 62,          // KEYCODE_SPACE
    key_enter = 66,          // KEYCODE_ENTER
    key_escape = 111,        // KEYCODE_ESCAPE
    key_left_shift = 59,     // KEYCODE_SHIFT_LEFT
    key_right_shift = 60,    // KEYCODE_SHIFT_RIGHT
    key_left_control = 113,  // KEYCODE_CTRL_LEFT
    key_right_control = 114, // KEYCODE_CTRL_RIGHT
    key_left_alt = 57,       // KEYCODE_ALT_LEFT
    key_right_alt = 58,      // KEYCODE_ALT_RIGHT
    key_tab = 61,            // KEYCODE_TAB
    key_backspace = 67,      // KEYCODE_DEL
    key_insert = 124,        // KEYCODE_INSERT
    key_delete = 112,        // KEYCODE_FORWARD_DEL
    key_home = 3,            // KEYCODE_HOME
    key_end = 123,           // KEYCODE_MOVE_END
    key_page_up = 92,        // KEYCODE_PAGE_UP
    key_page_down = 93,      // KEYCODE_PAGE_DOWN
    key_arrow_up = 19,       // KEYCODE_DPAD_UP
    key_arrow_down = 20,     // KEYCODE_DPAD_DOWN
    key_arrow_left = 21,     // KEYCODE_DPAD_LEFT
    key_arrow_right = 22,    // KEYCODE_DPAD_RIGHT
    key_f1 = 131,            // KEYCODE_F1
    key_f2 = 132,            // KEYCODE_F2
    key_f3 = 133,            // KEYCODE_F3
    key_f4 = 134,            // KEYCODE_F4
    key_f5 = 135,            // KEYCODE_F5
    key_f6 = 136,            // KEYCODE_F6
    key_f7 = 137,            // KEYCODE_F7
    key_f8 = 138,            // KEYCODE_F8
    key_f9 = 139,            // KEYCODE_F9
    key_f10 = 140,           // KEYCODE_F10
    key_f11 = 141,           // KEYCODE_F11
    key_f12 = 142,           // KEYCODE_F12
};

/**
 * @brief The `MouseButton` enum class represents the mouse buttons supported by Android.
 *
 * The values of this enum class are based on Android's MotionEvent button constants.
 *
 * @see https://developer.android.com/reference/android/view/MotionEvent
 */
enum class AGDKMouseButton : uint32_t
{
    button_left = 1 << 0,   // BUTTON_PRIMARY
    button_right = 1 << 1,  // BUTTON_SECONDARY
    button_middle = 1 << 2, // BUTTON_TERTIARY
    button_4 = 1 << 3,      // BUTTON_BACK
    button_5 = 1 << 4,      // BUTTON_FORWARD
    button_6 = 1 << 5,      // Additional button
    button_7 = 1 << 6,      // Additional button
    button_8 = 1 << 7,      // Additional button
};

} // namespace agdk
} // namespace platform
} // namespace saturn
