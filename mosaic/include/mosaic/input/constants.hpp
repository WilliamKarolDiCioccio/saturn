#pragma once

#include <chrono>

namespace saturn
{
namespace input
{

// Minimum time a key must be pressed to register
constexpr auto k_keyPressMinDuration = std::chrono::milliseconds(8);

// Time after releasing before a key is considered fully released
constexpr auto k_keyReleaseDuration = std::chrono::milliseconds(12);

// Maximum time between clicks to count as a double-click
constexpr auto k_doubleClickMaxInterval = std::chrono::milliseconds(500);

// Minimum time between clicks to count as a double-click
constexpr auto k_doubleClickMinInterval = std::chrono::milliseconds(20);

// Minimum time a key must be pressed to count as being held
constexpr auto k_keyHoldMinDuration = std::chrono::milliseconds(35);

// Sample rate for mouse wheel and cursor position data derivation
constexpr auto k_inputSamplingRate = std::chrono::milliseconds(16);

// Number of samples to keep for mouse wheel scroll and cursor position to derive data
constexpr auto k_mouseWheelNumSamples = 8;
constexpr auto k_mouseCursorNumSamples = 16;

// Maximum number of events to keep in the event history for all input types
constexpr auto k_eventHistoryMaxSize = 8;

} // namespace input
} // namespace saturn
