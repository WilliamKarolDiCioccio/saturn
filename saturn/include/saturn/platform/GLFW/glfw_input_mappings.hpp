#pragma once

#include <GLFW/glfw3.h>

namespace saturn
{
namespace platform
{
namespace glfw
{

enum class GLFWInputAction : uint32_t
{
    release = GLFW_RELEASE,
    press = GLFW_PRESS,
    repeat = GLFW_REPEAT,
};

/**
 * @brief The `KeyboardKey` enum class represents the keyboard keys supported by GLFW.
 *
 * The values of this enum class are the same as the GLFW key codes. This allows for easy mapping
 * between GLFW key codes and the `KeyboardKey` enum class.
 *
 * @see https://www.glfw.org/docs/latest/group__keys.html
 */
enum class GLFWKeyboardKey : uint32_t
{
    key_a = GLFW_KEY_A,
    key_b = GLFW_KEY_B,
    key_c = GLFW_KEY_C,
    key_d = GLFW_KEY_D,
    key_e = GLFW_KEY_E,
    key_f = GLFW_KEY_F,
    key_g = GLFW_KEY_G,
    key_h = GLFW_KEY_H,
    key_i = GLFW_KEY_I,
    key_j = GLFW_KEY_J,
    key_k = GLFW_KEY_K,
    key_l = GLFW_KEY_L,
    key_m = GLFW_KEY_M,
    key_n = GLFW_KEY_N,
    key_o = GLFW_KEY_O,
    key_p = GLFW_KEY_P,
    key_q = GLFW_KEY_Q,
    key_r = GLFW_KEY_R,
    key_s = GLFW_KEY_S,
    key_t = GLFW_KEY_T,
    key_u = GLFW_KEY_U,
    key_v = GLFW_KEY_V,
    key_w = GLFW_KEY_W,
    key_x = GLFW_KEY_X,
    key_y = GLFW_KEY_Y,
    key_z = GLFW_KEY_Z,
    key_0 = GLFW_KEY_0,
    key_1 = GLFW_KEY_1,
    key_2 = GLFW_KEY_2,
    key_3 = GLFW_KEY_3,
    key_4 = GLFW_KEY_4,
    key_5 = GLFW_KEY_5,
    key_6 = GLFW_KEY_6,
    key_7 = GLFW_KEY_7,
    key_8 = GLFW_KEY_8,
    key_9 = GLFW_KEY_9,
    key_space = GLFW_KEY_SPACE,
    key_enter = GLFW_KEY_ENTER,
    key_escape = GLFW_KEY_ESCAPE,
    key_left_shift = GLFW_KEY_LEFT_SHIFT,
    key_right_shift = GLFW_KEY_RIGHT_SHIFT,
    key_left_control = GLFW_KEY_LEFT_CONTROL,
    key_right_control = GLFW_KEY_RIGHT_CONTROL,
    key_left_alt = GLFW_KEY_LEFT_ALT,
    key_right_alt = GLFW_KEY_RIGHT_ALT,
    key_tab = GLFW_KEY_TAB,
    key_backspace = GLFW_KEY_BACKSPACE,
    key_insert = GLFW_KEY_INSERT,
    key_delete = GLFW_KEY_DELETE,
    key_home = GLFW_KEY_HOME,
    key_end = GLFW_KEY_END,
    key_page_up = GLFW_KEY_PAGE_UP,
    key_page_down = GLFW_KEY_PAGE_DOWN,
    key_arrow_up = GLFW_KEY_UP,
    key_arrow_down = GLFW_KEY_DOWN,
    key_arrow_left = GLFW_KEY_LEFT,
    key_arrow_right = GLFW_KEY_RIGHT,
    key_f1 = GLFW_KEY_F1,
    key_f2 = GLFW_KEY_F2,
    key_f3 = GLFW_KEY_F3,
    key_f4 = GLFW_KEY_F4,
    key_f5 = GLFW_KEY_F5,
    key_f6 = GLFW_KEY_F6,
    key_f7 = GLFW_KEY_F7,
    key_f8 = GLFW_KEY_F8,
    key_f9 = GLFW_KEY_F9,
    key_f10 = GLFW_KEY_F10,
    key_f11 = GLFW_KEY_F11,
    key_f12 = GLFW_KEY_F12,
};

/**
 * @brief The `MouseButton` enum class represents the mouse buttons supported by GLFW.
 *
 * The values of this enum class are the same as the GLFW mouse button codes. This allows for easy
 * mapping between GLFW mouse button codes and the `MouseButton` enum class.
 *
 * @see https://www.glfw.org/docs/latest/group__buttons.html
 */
enum class GLFWMouseButton : uint32_t
{
    button_left = GLFW_MOUSE_BUTTON_LEFT,
    button_right = GLFW_MOUSE_BUTTON_RIGHT,
    button_middle = GLFW_MOUSE_BUTTON_MIDDLE,
    button_4 = GLFW_MOUSE_BUTTON_4,
    button_5 = GLFW_MOUSE_BUTTON_5,
    button_6 = GLFW_MOUSE_BUTTON_6,
    button_7 = GLFW_MOUSE_BUTTON_7,
    button_8 = GLFW_MOUSE_BUTTON_8,
};

} // namespace glfw
} // namespace platform
} // namespace saturn
