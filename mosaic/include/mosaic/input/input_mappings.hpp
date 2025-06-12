#pragma once

#include "mosaic/defines.hpp"

#if defined(MOSAIC_PLATFORM_DESKTOP) || defined(MOSAIC_PLATFORM_WEB)
#include "mosaic/platform/GLFW/glfw_input_mappings.hpp"
#else
#include "mosaic/platform/AGDK/agdk_input_mappings.hpp"
#endif

namespace mosaic
{
namespace input
{

#if defined(MOSAIC_PLATFORM_DESKTOP) || defined(MOSAIC_PLATFORM_WEB)
using KeyboardKey = mosaic::platform::glfw::GLFWKeyboardKey;
using MouseButton = mosaic::platform::glfw::GLFWMouseButton;
using InputAction = mosaic::platform::glfw::GLFWInputAction;
#else
using KeyboardKey = mosaic::platform::agdk::AGDKKeyboardKey;
using MouseButton = mosaic::platform::agdk::AGDKMouseButton;
using InputAction = mosaic::platform::agdk::AGDKInputAction;
#endif

/**
 * @brief The `c_keyboardKeys` array contains all the keyboard keys.
 *
 * This allows iterating over all the keyboard keys and checking their state.
 */
constexpr std::array<KeyboardKey, 349> c_keyboardKeys = {
    KeyboardKey::key_a,
    KeyboardKey::key_b,
    KeyboardKey::key_c,
    KeyboardKey::key_d,
    KeyboardKey::key_e,
    KeyboardKey::key_f,
    KeyboardKey::key_g,
    KeyboardKey::key_h,
    KeyboardKey::key_i,
    KeyboardKey::key_j,
    KeyboardKey::key_k,
    KeyboardKey::key_l,
    KeyboardKey::key_m,
    KeyboardKey::key_n,
    KeyboardKey::key_o,
    KeyboardKey::key_p,
    KeyboardKey::key_q,
    KeyboardKey::key_r,
    KeyboardKey::key_s,
    KeyboardKey::key_t,
    KeyboardKey::key_u,
    KeyboardKey::key_v,
    KeyboardKey::key_w,
    KeyboardKey::key_x,
    KeyboardKey::key_y,
    KeyboardKey::key_z,
    KeyboardKey::key_0,
    KeyboardKey::key_1,
    KeyboardKey::key_2,
    KeyboardKey::key_3,
    KeyboardKey::key_4,
    KeyboardKey::key_5,
    KeyboardKey::key_6,
    KeyboardKey::key_7,
    KeyboardKey::key_8,
    KeyboardKey::key_9,
    KeyboardKey::key_space,
    KeyboardKey::key_enter,
    KeyboardKey::key_escape,
    KeyboardKey::key_left_shift,
    KeyboardKey::key_right_shift,
    KeyboardKey::key_left_control,
    KeyboardKey::key_right_control,
    KeyboardKey::key_left_alt,
    KeyboardKey::key_right_alt,
    KeyboardKey::key_tab,
    KeyboardKey::key_backspace,
    KeyboardKey::key_insert,
    KeyboardKey::key_delete,
    KeyboardKey::key_home,
    KeyboardKey::key_end,
    KeyboardKey::key_page_up,
    KeyboardKey::key_page_down,
    KeyboardKey::key_arrow_up,
    KeyboardKey::key_arrow_down,
    KeyboardKey::key_arrow_left,
    KeyboardKey::key_arrow_right,
    KeyboardKey::key_f1,
    KeyboardKey::key_f2,
    KeyboardKey::key_f3,
    KeyboardKey::key_f4,
    KeyboardKey::key_f5,
    KeyboardKey::key_f6,
    KeyboardKey::key_f7,
    KeyboardKey::key_f8,
    KeyboardKey::key_f9,
    KeyboardKey::key_f10,
    KeyboardKey::key_f11,
    KeyboardKey::key_f12,
};

/**
 * @brief The `c_mouseButtons` array contains all the mouse buttons.
 *
 * This allows iterating over all the mouse buttons and checking their state.
 */
constexpr std::array<MouseButton, 8> c_mouseButtons = {
    MouseButton::button_left, MouseButton::button_right, MouseButton::button_middle,
    MouseButton::button_4,    MouseButton::button_5,     MouseButton::button_6,
    MouseButton::button_7,    MouseButton::button_8,
};

} // namespace input
} // namespace mosaic
