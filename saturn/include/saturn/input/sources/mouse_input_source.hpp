#pragma once

#include "input_source.hpp"

#include <pieces/containers/circular_buffer.hpp>

namespace saturn
{
namespace input
{

/**
 * @brief The `MouseInputSource` class is an abstract base class for mouse input sources.
 *
 * @note This class is not meant to be instantiated directly. Instead, use the
 * `MouseInputSource::create()` factory method to create an instance of a concrete mouse input
 * source implementation.

 * @see InputSource
 */
class SATURN_API MouseInputSource : public InputSource
{
   protected:
    struct MouseWheelScrollSample
    {
        glm::vec2 offset;
        std::chrono::time_point<std::chrono::high_resolution_clock> timestamp;

        MouseWheelScrollSample()
            : offset(0.0f, 0.0f), timestamp(std::chrono::high_resolution_clock::now()) {};

        MouseWheelScrollSample(
            const glm::vec2& _offset,
            std::chrono::time_point<std::chrono::high_resolution_clock> _timestamp)
            : offset(_offset), timestamp(_timestamp) {};
    };

    struct MouseCursorPosSample
    {
        glm::vec2 position;
        std::chrono::time_point<std::chrono::high_resolution_clock> timestamp;

        MouseCursorPosSample()
            : position(0.0f, 0.0f), timestamp(std::chrono::high_resolution_clock::now()) {};

        MouseCursorPosSample(const glm::vec2& _position,
                             std::chrono::time_point<std::chrono::high_resolution_clock> _timestamp)
            : position(_position), timestamp(_timestamp) {};
    };

    // Events
    std::array<pieces::CircularBuffer<MouseButtonEvent, k_eventHistoryMaxSize>,
               c_mouseButtons.size()>
        m_mouseButtonEvents;
    pieces::CircularBuffer<MouseWheelScrollEvent, k_eventHistoryMaxSize> m_mouseScrollEvents;
    pieces::CircularBuffer<MouseCursorMoveEvent, k_eventHistoryMaxSize> m_cursorMoveEvents;

    // Samples
    pieces::CircularBuffer<MouseWheelScrollSample, k_mouseWheelNumSamples>
        m_mouseScrollWheelSamples;
    pieces::CircularBuffer<MouseCursorPosSample, k_mouseCursorNumSamples> m_cursorPosSamples;

    // Derived
    glm::vec2 m_wheelOffset;
    glm::vec2 m_wheelDelta;
    glm::vec2 m_cursorPosition;
    glm::vec2 m_cursorDelta;

    // Customizable parameters
    glm::vec2 m_wheelSensitivity;
    glm::vec2 m_cursorSensitivity;

   public:
    MouseInputSource(window::Window* _window);
    virtual ~MouseInputSource() = default;

    static std::unique_ptr<MouseInputSource> create(window::Window* _window);

   public:
    virtual pieces::RefResult<InputSource, std::string> initialize() override = 0;
    virtual void shutdown() override = 0;

    virtual void pollDevice() override = 0;
    void processInput() override;

    [[nodiscard]] inline const glm::vec2 getWheelOffset() const { return m_wheelOffset; }

    [[nodiscard]] inline const glm::vec2 getWheelDelta() const
    {
        return m_wheelDelta * m_wheelSensitivity;
    }

    [[nodiscard]] inline const glm::vec2 getCursorPosition() const { return m_cursorPosition; }

    [[nodiscard]] inline const glm::vec2 getCursorDelta() const
    {
        return m_cursorDelta * m_cursorSensitivity;
    }

    [[nodiscard]] const glm::vec2 getAveragedWheelDeltas() const;
    [[nodiscard]] const glm::vec2 getWheelSpeed() const;
    [[nodiscard]] const glm::vec2 getWheelAcceleration() const;
    [[nodiscard]] GestureDirection getCursorGestureDirection() const;

    [[nodiscard]] const glm::vec2 getAveragedCursorDeltas() const;
    [[nodiscard]] const glm::vec2 getCursorSpeed() const;
    [[nodiscard]] double getCursorLinearSpeed() const;
    [[nodiscard]] const glm::vec2 getCursorAcceleration() const;
    [[nodiscard]] double getCursorLinearAcceleration() const;

    [[nodiscard]] inline const MouseWheelScrollEvent& getWheelScrollEvent() const
    {
        return m_mouseScrollEvents.front().unwrap();
    }

    [[nodiscard]] inline auto getWheelScrollEventHistory() const
        -> decltype(m_mouseScrollEvents.data())
    {
        return m_mouseScrollEvents.data();
    }

    [[nodiscard]] inline const MouseCursorMoveEvent& getCursorPosEvent() const
    {
        return m_cursorMoveEvents.front().unwrap();
    }

    [[nodiscard]] inline auto getCursorPosEventHistory() const
        -> decltype(m_cursorMoveEvents.data())
    {
        return m_cursorMoveEvents.data();
    }

    [[nodiscard]] inline const MouseButtonEvent& getButtonEvent(MouseButton _button) const
    {
        return m_mouseButtonEvents[static_cast<uint32_t>(_button)].front().unwrap();
    }

    [[nodiscard]] inline auto getButtonsEventHistory(MouseButton _button) const
        -> decltype(m_mouseButtonEvents[0].data())
    {
        return m_mouseButtonEvents[static_cast<uint32_t>(_button)].data();
    }

    inline void setWheelSensitivity(const glm::vec2& _sensitivity)
    {
        m_wheelSensitivity = glm::clamp(_sensitivity, 0.1f, 10.f);
    }

    [[nodiscard]] inline const glm::vec2& getWheelSensitivity() const { return m_wheelSensitivity; }

    inline void setCursorSensitivity(const glm::vec2& _sensitivity)
    {
        m_cursorSensitivity = glm::clamp(_sensitivity, 0.1f, 10.f);
    }

    [[nodiscard]] inline const glm::vec2& getCursorSensitivity() const
    {
        return m_cursorSensitivity;
    }

   protected:
    [[nodiscard]] virtual InputAction queryButtonState(MouseButton _button) const = 0;
    [[nodiscard]] virtual glm::vec2 queryCursorPosition() const = 0;
    [[nodiscard]] virtual glm::vec2 queryWheelOffset() = 0;
};

} // namespace input
} // namespace saturn
