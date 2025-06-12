#pragma once

#include <unordered_map>
#include <algorithm>

#include <pieces/circular_buffer.hpp>

#include "mosaic/core/timer.hpp"
#include "mosaic/utils/enum.hpp"

#include "events.hpp"
#include "raw_input_handler.hpp"

namespace mosaic
{
namespace input
{

// Minimum time a key must be pressed to register
constexpr auto KEY_PRESS_MIN_DURATION = 8ms;

// Time after releasing before a key is considered fully released
constexpr auto KEY_RELEASE_DURATION = 12ms;

// Maximum time between clicks to count as a double-click
constexpr auto DOUBLE_CLICK_MAX_INTERVAL = 500ms;

// Minimum time between clicks to count as a double-click
constexpr auto DOUBLE_CLICK_MIN_INTERVAL = 20ms;

// Minimum time a key must be pressed to count as being held
constexpr auto KEY_HOLD_MIN_DURATION = 35ms;

// Number of samples to keep for mouse wheel scroll and cursor position to derive data
constexpr auto MOUSE_WHEEL_NUM_SAMPLES = 8;
constexpr auto MOUSE_CURSOR_NUM_SAMPLES = 16;

enum MovementDirection : uint32_t
{
    none = 0,
    up = 1 << 0,
    down = 1 << 1,
    left = 1 << 2,
    right = 1 << 3
};

MOSAIC_DEFINE_ENUM_FLAGS_OPERATORS(MovementDirection)

/**
 * @brief The `InputArena` is responsible for processing raw inputs into high-level events.
 *
 * `InputArena` responsibilities include:
 *
 * - Detecting and processing keyboard, mouse button, mouse wheel scroll, and cursor position events
 *   and generating high-level events from them.
 *
 * - Deriving additional information from raw input data, such as speed, acceleration, and
 *   movement direction for mouse wheel scroll and cursor position.
 *
 * - Enriching events with meta-data such as timestamps and durations.
 *
 * @see RawInputHandler
 */
class InputArena
{
   private:
    struct MouseWheelScrollSample
    {
        glm::vec2 rawScrollOffset;
        std::chrono::time_point<std::chrono::high_resolution_clock> timestamp;

        MouseWheelScrollSample(
            const glm::vec2& _rawScrollOffset,
            std::chrono::time_point<std::chrono::high_resolution_clock> _timestamp)
            : rawScrollOffset(_rawScrollOffset), timestamp(_timestamp) {};
    };

    struct MouseCursorPosSample
    {
        glm::vec2 pos;
        glm::vec2 lastPos;
        std::chrono::time_point<std::chrono::high_resolution_clock> timestamp;

        MouseCursorPosSample(const glm::vec2& _pos, const glm::vec2& _lastPos,
                             std::chrono::time_point<std::chrono::high_resolution_clock> _timestamp)
            : pos(_pos), lastPos(_lastPos), timestamp(_timestamp) {};

        const glm::vec2 getDelta() const { return pos - lastPos; };
    };

    core::Timer m_timer;
    uint64_t m_pollCount = 0;
    std::unique_ptr<RawInputHandler> m_rawInputHandler;

    // Events
    std::array<KeyboardKeyEvent, c_keyboardKeys.size()> m_keyboardKeyEvents;
    std::array<MouseButtonEvent, c_mouseButtons.size()> m_mouseButtonEvents;
    MouseWheelScrollEvent m_mouseScrollEvent;
    MouseCursorPosEvent m_cursorPosEvent;

    // Discrete samples for mouse wheel scroll and cursor position
    pieces::CircularBuffer<MouseWheelScrollSample> m_mouseScrollWheelSamples;
    pieces::CircularBuffer<MouseCursorPosSample> m_cursorPosSamples;

   public:
    InputArena(core::Window* _window);

    InputArena(const InputArena&) = delete;
    InputArena& operator=(const InputArena&) = delete;
    InputArena(InputArena&&) = delete;
    InputArena& operator=(InputArena&&) = delete;

   public:
    pieces::RefResult<InputArena, std::string> initialize();
    void shutdown();

    void update();

    inline const MouseWheelScrollEvent& getMouseWheelScrollEvent() const
    {
        return m_mouseScrollEvent;
    }

    inline const MouseCursorPosEvent& getMouseCursorPosEvent() const { return m_cursorPosEvent; }

    inline const std::array<KeyboardKeyEvent, c_keyboardKeys.size()>& getKeyboardKeysEvents() const
    {
        return m_keyboardKeyEvents;
    }

    inline const std::array<MouseButtonEvent, c_mouseButtons.size()>& getMouseButtonsEvents() const
    {
        return m_mouseButtonEvents;
    }

    inline const KeyboardKeyEvent& getKeyboardKeyEvent(KeyboardKey _key) const
    {
        return m_keyboardKeyEvents[static_cast<uint32_t>(_key)];
    }

    inline const MouseButtonEvent& getMouseButtonEvent(MouseButton _button) const
    {
        return m_mouseButtonEvents[static_cast<uint32_t>(_button)];
    }

    const glm::vec2 getWheelOffset() const;
    const glm::vec2 getAveragedWheelDeltas() const;
    const glm::vec2 getWheelSpeed() const;
    const glm::vec2 getWheelAccelleration() const;
    const glm::vec2 getCursorPosition() const;
    const glm::vec2 getCursorDelta() const;
    const glm::vec2 getAveragedCursorDeltas() const;
    const glm::vec2 getCursorSpeed() const;
    const double getCursorLinearSpeed() const;
    const glm::vec2 getCursorAccelleration() const;
    const double getCursorLinearAccelleration() const;
    const MovementDirection getCursorMovementDirection() const;
};

} // namespace input
} // namespace mosaic
