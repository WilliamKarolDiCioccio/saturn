#include "saturn/input/sources/mouse_input_source.hpp"

#if defined(SATURN_PLATFORM_DESKTOP) || defined(SATURN_PLATFORM_WEB)
#include "platform/GLFW/glfw_mouse_input_source.hpp"
#endif

using namespace std::chrono_literals;

namespace saturn
{
namespace input
{

MouseInputSource::MouseInputSource(window::Window* _window)
    : InputSource(_window),
      m_wheelOffset(0.f),
      m_wheelDelta(0.f),
      m_cursorPosition(0.f),
      m_cursorDelta(0.f),
      m_wheelSensitivity(1.0f),
      m_cursorSensitivity(1.0f)
{
    auto currentTime = std::chrono::high_resolution_clock::now();

    MouseButtonEvent dummyButtonEvent = {
        ActionableState::none,
        currentTime,
        m_pollCount,
        0ms,
    };

    for (auto& buffer : m_mouseButtonEvents)
    {
        buffer.push(dummyButtonEvent);
    }

    m_mouseScrollEvents.push(MouseWheelScrollEvent(glm::vec2(0.f), glm::vec2(0.f), currentTime,
                                                   m_pollCount, InputEventType::none));

    m_mouseScrollWheelSamples.push(MouseWheelScrollSample(glm::vec2(0.f), currentTime));

    m_cursorMoveEvents.push(MouseCursorMoveEvent(glm::vec2(0.f), glm::vec2(0.f), currentTime,
                                                 m_pollCount, InputEventType::none));

    m_cursorPosSamples.push(MouseCursorPosSample(glm::vec2(0.f), currentTime));
};

std::unique_ptr<MouseInputSource> MouseInputSource::create(window::Window* _window)
{
#if defined(SATURN_PLATFORM_DESKTOP) || defined(SATURN_PLATFORM_WEB)
    return std::make_unique<platform::glfw::GLFWMouseInputSource>(_window);
#else
    throw std::runtime_error("Mouse input source not supported on mobile devices");
#endif
}

void MouseInputSource::processInput()
{
    auto currentTime = std::chrono::high_resolution_clock::now();

    if (!isActive())
    {
        m_cursorPosSamples.push(MouseCursorPosSample(glm::vec2(0.f), currentTime));
        m_mouseScrollWheelSamples.push(MouseWheelScrollSample(glm::vec2(0.f), currentTime));

        return;
    }

    pollDevice();

    // Fixed mouse button processing
    for (const auto& button : c_mouseButtons)
    {
        const InputAction action = queryButtonState(button);
        auto& eventQueue = m_mouseButtonEvents[static_cast<uint32_t>(button)];

        const auto lastEvent = eventQueue.front().unwrap();

        const bool wasDown = utils::hasFlag(lastEvent.state, ActionableState::press) ||
                             utils::hasFlag(lastEvent.state, ActionableState::hold);

        auto timeSinceLastEvent = std::chrono::duration_cast<std::chrono::milliseconds>(
            currentTime - lastEvent.metadata.timestamp);

        MouseButtonEvent currEvent;

        if (action == InputAction::press)
        {
            if (wasDown)
            {
                if (utils::hasFlag(lastEvent.state, ActionableState::press) &&
                    timeSinceLastEvent >= k_keyHoldMinDuration)
                {
                    currEvent = MouseButtonEvent{
                        ActionableState::hold,
                        lastEvent.metadata.timestamp, // Keep original press timestamp
                        m_pollCount,
                        timeSinceLastEvent, // Duration since press
                    };

                    eventQueue.push(currEvent);
                }
                else if (utils::hasFlag(lastEvent.state, ActionableState::hold))
                {
                    auto totalHoldDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
                        currentTime - lastEvent.metadata.timestamp + lastEvent.metadata.duration);

                    currEvent = MouseButtonEvent{
                        ActionableState::hold,
                        lastEvent.metadata.timestamp, // Keep original press timestamp
                        m_pollCount,
                        totalHoldDuration, // Total duration since press
                    };

                    eventQueue.push(currEvent);
                }
            }
            else
            {
                if (utils::hasFlag(lastEvent.state, ActionableState::release) &&
                    timeSinceLastEvent >= k_doubleClickMinInterval &&
                    timeSinceLastEvent <= k_doubleClickMaxInterval)
                {
                    currEvent = MouseButtonEvent{
                        ActionableState::press | ActionableState::double_press,
                        currentTime,
                        m_pollCount,
                        std::chrono::milliseconds(0), // Press is instantaneous
                    };
                }
                else
                {
                    currEvent = MouseButtonEvent{
                        ActionableState::press,
                        currentTime,
                        m_pollCount,
                        std::chrono::milliseconds(0), // Press is instantaneous
                    };
                }

                eventQueue.push(currEvent);
            }
        }
        else if (action == InputAction::release)
        {
            if (wasDown)
            {
                auto totalPressDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
                    currentTime - lastEvent.metadata.timestamp);

                if (totalPressDuration >= k_keyReleaseDuration)
                {
                    currEvent = MouseButtonEvent{
                        ActionableState::release,
                        currentTime,
                        m_pollCount,
                        totalPressDuration, // Duration from press to release
                    };

                    eventQueue.push(currEvent);
                }
            }
        }
    }

    {
        const auto& pos = queryCursorPosition();

        m_cursorDelta = pos - m_cursorPosition;
        m_cursorPosition = pos;

        const auto lastSample = m_cursorPosSamples.back().unwrap();

        if (currentTime - lastSample.timestamp > k_inputSamplingRate)
        {
            MouseCursorPosSample sample(pos, currentTime);

            m_cursorPosSamples.push(sample);
        }

        const auto lastEvent = m_cursorMoveEvents.back().unwrap();

        if (utils::hasFlag(lastEvent.metadata.type, InputEventType::end))
        {
            MouseCursorMoveEvent cursorPosEvent = {
                pos, glm::vec2(0.0f), currentTime, m_pollCount, InputEventType::start,
            };

            m_cursorMoveEvents.push(cursorPosEvent);
        }
        else
        {
            const auto delta = pos - lastEvent.position;

            MouseCursorMoveEvent cursorPosEvent = {
                pos, delta, currentTime, m_pollCount, InputEventType::end,
            };

            m_cursorMoveEvents.push(cursorPosEvent);
        }
    }

    {
        const auto& offset = queryWheelOffset();

        m_wheelDelta = offset - m_wheelOffset;
        m_wheelOffset = offset;

        const auto lastSample = m_mouseScrollWheelSamples.back().unwrap();

        if (currentTime - lastSample.timestamp > k_inputSamplingRate)
        {
            MouseWheelScrollSample sample(glm::vec2(offset), currentTime);

            m_mouseScrollWheelSamples.push(sample);
        }

        const auto lastEvent = m_mouseScrollEvents.back().unwrap();

        if (utils::hasFlag(lastEvent.metadata.type, InputEventType::end))
        {
            MouseWheelScrollEvent scrollEvent = {
                offset, glm::vec2(0.0f), currentTime, m_pollCount, InputEventType::start,
            };

            m_mouseScrollEvents.push(scrollEvent);
        }
        else
        {
            const auto delta = offset - lastEvent.offset;

            MouseWheelScrollEvent scrollEvent = {
                offset, delta, currentTime, m_pollCount, InputEventType::end,
            };

            m_mouseScrollEvents.push(scrollEvent);
        }
    }
}

const glm::vec2 MouseInputSource::getAveragedWheelDeltas() const
{
    if (m_mouseScrollWheelSamples.empty()) return {0.0, 0.0};

    double xSum = 0.0;
    double ySum = 0.0;

    glm::vec2 sum(0.f);

    for (size_t i = 0; i < m_mouseScrollWheelSamples.size(); ++i)
    {
        // Offset is already a delta as wheel can scroll inifinitely
        sum += m_mouseScrollWheelSamples[i].offset;
    }

    glm::vec2 averaged = sum / static_cast<float>(m_mouseScrollWheelSamples.size());

    return averaged * m_wheelSensitivity;
}

const glm::vec2 MouseInputSource::getWheelSpeed() const
{
    if (m_mouseScrollWheelSamples.empty()) return {0.0, 0.0};

    auto oldestTime = m_mouseScrollWheelSamples.front().unwrap().timestamp;
    auto newestTime = m_mouseScrollWheelSamples.back().unwrap().timestamp;
    auto timeSpan = std::chrono::duration<float>(newestTime - oldestTime).count();

    if (timeSpan == 0) return {0.0, 0.0};

    glm::vec2 averaged = getAveragedWheelDeltas(); // already scaled by sensitivity

    return averaged / timeSpan;
}

const glm::vec2 MouseInputSource::getWheelAcceleration() const
{
    if (m_mouseScrollWheelSamples.empty()) return glm::vec2(0.f);

    auto oldestTime = m_mouseScrollWheelSamples.front().unwrap().timestamp;
    auto newestTime = m_mouseScrollWheelSamples.back().unwrap().timestamp;
    auto timeSpan = std::chrono::duration<float>(newestTime - oldestTime).count();

    if (timeSpan == 0) return glm::vec2(0.f);

    glm::vec2 speed = getWheelSpeed(); // already scaled appropriately

    return speed / timeSpan;
}

const glm::vec2 MouseInputSource::getAveragedCursorDeltas() const
{
    if (m_cursorPosSamples.empty()) return glm::vec2(0.f);

    glm::vec2 lastPos(0.f);
    glm::vec2 sum(0.f);

    for (size_t i = 0; i < m_cursorPosSamples.size(); ++i)
    {
        const auto currPos = m_cursorPosSamples[i].position;

        lastPos = currPos;
        sum += currPos - lastPos;
    }

    glm::vec2 averaged = sum / static_cast<float>(m_cursorPosSamples.size());

    return averaged * m_cursorSensitivity;
}

const glm::vec2 MouseInputSource::getCursorSpeed() const
{
    if (m_cursorPosSamples.empty()) return glm::vec2(0.f);

    auto oldestTime = m_cursorPosSamples.front().unwrap().timestamp;
    auto newestTime = m_cursorPosSamples.back().unwrap().timestamp;
    auto timeSpan = std::chrono::duration<float>(newestTime - oldestTime).count();

    if (timeSpan == 0) return glm::vec2(0.f);

    glm::vec2 averaged = getAveragedCursorDeltas(); // already scaled by sensitivity

    return averaged / timeSpan;
};

double MouseInputSource::getCursorLinearSpeed() const
{
    return glm::length(getCursorSpeed()); // already scaled by sensitivity
}

const glm::vec2 MouseInputSource::getCursorAcceleration() const
{
    if (m_cursorPosSamples.empty()) return glm::vec2(0.f);

    auto oldestTime = m_cursorPosSamples.front().unwrap().timestamp;
    auto newestTime = m_cursorPosSamples.back().unwrap().timestamp;
    auto timeSpan = std::chrono::duration<float>(newestTime - oldestTime).count();

    if (timeSpan == 0) return glm::vec2(0.f);

    const glm::vec2 speed = getCursorSpeed(); // already scaled by sensitivity

    return speed / timeSpan;
}

double MouseInputSource::getCursorLinearAcceleration() const
{
    return glm::length(getCursorAcceleration());
}

GestureDirection MouseInputSource::getCursorGestureDirection() const
{
    const glm::vec2 delta = getAveragedCursorDeltas();

    if (glm::length(delta) < 128)
    {
        return GestureDirection::none;
    }

    GestureDirection dir = GestureDirection::none;

    const float absX = std::abs(delta.x);
    const float absY = std::abs(delta.y);

    if (absX > 128)
    {
        if (delta.x < 0)
        {
            dir |= GestureDirection::left;
        }
        else
        {
            dir |= GestureDirection::right;
        }
    }

    if (absY > 128)
    {
        if (delta.y < 0)
        {
            dir |= GestureDirection::up;
        }
        else
        {
            dir |= GestureDirection::down;
        }
    }

    return dir;
}

} // namespace input
} // namespace saturn
