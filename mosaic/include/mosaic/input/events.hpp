#pragma once

#include <chrono>
#include <glm/vec2.hpp>
#include <glm/geometric.hpp>

#include "mosaic/utils/enum.hpp"

namespace mosaic
{
namespace input
{

using namespace std::chrono_literals;

// Keyboard keys and mouse buttons share the same possible states, so we can use the same enum.
enum class KeyButtonState : uint32_t
{
    none = 0,
    release = 1 << 0,
    press = 1 << 1,
    hold = 1 << 2,
    double_press = 1 << 3
};

MOSAIC_DEFINE_ENUM_FLAGS_OPERATORS(KeyButtonState)

/**
 * @brief The `InputEventMetadata` struct contains meta-data for input events.
 *
 * It's the `InputArena`'s responsibility to fill this struct with the correct data.
 *
 * @see InputArena
 */
struct InputEventMetadata
{
    std::chrono::time_point<std::chrono::high_resolution_clock> timestamp;
    std::chrono::duration<double> duration;
    uint64_t pollCount = 0;

    InputEventMetadata(std::chrono::time_point<std::chrono::high_resolution_clock> _now,
                       std::chrono::duration<double> _duration, uint64_t _pollCount = 0)
        : timestamp(_now), duration(_duration), pollCount(_pollCount) {};
};

struct KeyboardKeyEvent
{
    InputEventMetadata metadata;
    KeyButtonState state;
    KeyButtonState lastSignificantState;

    KeyboardKeyEvent()
        : metadata(std::chrono::high_resolution_clock::now(), 0ms, 0),
          state(KeyButtonState::none),
          lastSignificantState(KeyButtonState::none) {};

    KeyboardKeyEvent(KeyButtonState _state, KeyButtonState _lastSignificantState,
                     std::chrono::time_point<std::chrono::high_resolution_clock> _now,
                     uint64_t _pollCount, std::chrono::duration<double> _duration = 0ms)
        : metadata(_now, _duration, _pollCount),
          state(_state),
          lastSignificantState(_lastSignificantState) {};
};

struct MouseButtonEvent
{
    InputEventMetadata metadata;
    KeyButtonState state;
    KeyButtonState lastSignificantState;

    MouseButtonEvent()
        : metadata(std::chrono::high_resolution_clock::now(), 0ms, 0),
          state(KeyButtonState::none),
          lastSignificantState(KeyButtonState::none) {};

    MouseButtonEvent(KeyButtonState _state, KeyButtonState _lastSignificantState,
                     std::chrono::time_point<std::chrono::high_resolution_clock> _now,
                     std::chrono::duration<double> _duration = 0ms)
        : metadata(_now, _duration), state(_state), lastSignificantState(_lastSignificantState) {};
};

struct MouseCursorPosEvent
{
    InputEventMetadata metadata;
    glm::vec2 rawPos;
    glm::vec2 lastRawPos;

    MouseCursorPosEvent()
        : metadata(std::chrono::high_resolution_clock::now(), 0ms, 0),
          rawPos(0.0f),
          lastRawPos(0.0f) {};

    MouseCursorPosEvent(glm::vec2 _rawPos, glm::vec2 _lastRawPos,
                        std::chrono::time_point<std::chrono::high_resolution_clock> _now,
                        uint64_t _pollCount, std::chrono::duration<double> _duration = 0ms)
        : metadata(_now, _duration, _pollCount), rawPos(_rawPos), lastRawPos(_lastRawPos) {};
};

struct MouseWheelScrollEvent
{
    InputEventMetadata metadata;
    glm::vec2 rawScrollOffset;

    MouseWheelScrollEvent()
        : metadata(std::chrono::high_resolution_clock::now(), 0ms, 0), rawScrollOffset(0.0f) {};

    MouseWheelScrollEvent(glm::vec2 _rawScrollOffset,
                          std::chrono::time_point<std::chrono::high_resolution_clock> _now,
                          uint64_t _pollCount, std::chrono::duration<double> _duration = 0ms)
        : metadata(_now, _duration, _pollCount), rawScrollOffset(_rawScrollOffset) {};
};

} // namespace input
} // namespace mosaic
