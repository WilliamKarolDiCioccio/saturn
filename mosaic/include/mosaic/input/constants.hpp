#pragma once

#include <chrono>

namespace mosaic
{
namespace input
{

// Minimum time a key must be pressed to register
constexpr auto KEY_PRESS_MIN_DURATION = std::chrono::milliseconds(8);

// Time after releasing before a key is considered fully released
constexpr auto KEY_RELEASE_DURATION = std::chrono::milliseconds(12);

// Maximum time between clicks to count as a double-click
constexpr auto DOUBLE_CLICK_MAX_INTERVAL = std::chrono::milliseconds(500);

// Minimum time between clicks to count as a double-click
constexpr auto DOUBLE_CLICK_MIN_INTERVAL = std::chrono::milliseconds(20);

// Minimum time a key must be pressed to count as being held
constexpr auto KEY_HOLD_MIN_DURATION = std::chrono::milliseconds(35);

// Sample rate for mouse wheel and cursor position data derivation
constexpr auto INPUT_SAMPLING_RATE = std::chrono::milliseconds(16);

// Number of samples to keep for mouse wheel scroll and cursor position to derive data
constexpr auto MOUSE_WHEEL_NUM_SAMPLES = 8;
constexpr auto MOUSE_CURSOR_NUM_SAMPLES = 16;

// Maximum number of events to keep in the event history for all input types
constexpr auto EVENT_HISTORY_MAX_SIZE = 8;

} // namespace input
} // namespace mosaic
