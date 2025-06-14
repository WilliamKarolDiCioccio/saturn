#include "mosaic/input/input_arena.hpp"

namespace mosaic
{
namespace input
{

InputArena::InputArena(window::Window* _window)
    : m_rawInputHandler(RawInputHandler::create(_window)),
      m_mouseScrollWheelSamples(MOUSE_WHEEL_NUM_SAMPLES),
      m_cursorPosSamples(MOUSE_CURSOR_NUM_SAMPLES) {};

pieces::RefResult<InputArena, std::string> InputArena::initialize()
{
    m_cursorPosSamples.push(MouseCursorPosSample{
        glm::vec2(0.f),
        glm::vec2(0.f),
        std::chrono::high_resolution_clock::now(),
    });

    m_mouseScrollWheelSamples.push(MouseWheelScrollSample{
        glm::vec2(0.f),
        std::chrono::high_resolution_clock::now(),
    });

    auto result = m_rawInputHandler->initialize();

    if (result.isErr())
    {
        return pieces::ErrRef<InputArena, std::string>(std::move(result.error()));
    }

    return pieces::OkRef<InputArena, std::string>(*this);
}

void InputArena::shutdown() { m_rawInputHandler->shutdown(); }

void InputArena::update()
{
    using namespace std::chrono_literals;

    auto currentTime = std::chrono::high_resolution_clock::now();

    ++m_pollCount;

    for (const auto& key : c_keyboardKeys)
    {
        if (!m_rawInputHandler->isActive()) break;

        const InputAction action = m_rawInputHandler->getKeyboardKeyInput(key);

        auto& keyEvent = m_keyboardKeyEvents[static_cast<uint32_t>(key)];

        const bool wasDown = utils::hasFlag(keyEvent.state, KeyButtonState::press) ||
                             utils::hasFlag(keyEvent.state, KeyButtonState::hold);

        auto actionDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
            currentTime - keyEvent.metadata.timestamp);

        if (action == InputAction::release)
        {
            if (actionDuration < KEY_RELEASE_DURATION) continue;

            if (wasDown)
            {
                keyEvent = KeyboardKeyEvent{
                    KeyButtonState::release,
                    KeyButtonState::release,
                    currentTime,
                    m_pollCount,
                };
            }
            else
            {
                keyEvent = KeyboardKeyEvent{
                    KeyButtonState::none,        keyEvent.lastSignificantState,
                    keyEvent.metadata.timestamp, m_pollCount,
                    keyEvent.metadata.duration,
                };
            }
        }
        else if (action == InputAction::press)
        {
            if (utils::hasFlag(keyEvent.lastSignificantState, KeyButtonState::release) &&
                actionDuration > DOUBLE_CLICK_MIN_INTERVAL &&
                actionDuration < DOUBLE_CLICK_MAX_INTERVAL)
            {
                keyEvent = KeyboardKeyEvent{
                    KeyButtonState::press | KeyButtonState::double_press,
                    KeyButtonState::press | KeyButtonState::double_press,
                    currentTime,
                    m_pollCount,
                    actionDuration,
                };
            }
            else if (wasDown && actionDuration < KEY_HOLD_MIN_DURATION)
            {
                keyEvent = KeyboardKeyEvent{
                    KeyButtonState::hold,
                    KeyButtonState::hold,
                    currentTime,
                    m_pollCount,
                    keyEvent.metadata.duration + currentTime - keyEvent.metadata.timestamp,
                };
            }
            else
            {
                keyEvent = KeyboardKeyEvent{
                    KeyButtonState::press,
                    KeyButtonState::press,
                    currentTime,
                    m_pollCount,
                };
            }
        }
    }

    for (const auto& button : c_mouseButtons)
    {
        if (!m_rawInputHandler->isActive()) break;

        auto action = m_rawInputHandler->getMouseButtonInput(button);

        auto& buttonEvent = m_mouseButtonEvents[static_cast<uint32_t>(button)];

        const bool wasDown = utils::hasFlag(buttonEvent.state, KeyButtonState::press) ||
                             utils::hasFlag(buttonEvent.state, KeyButtonState::hold);

        auto actionDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
            currentTime - buttonEvent.metadata.timestamp);

        if (action == InputAction::release)
        {
            if (actionDuration < KEY_RELEASE_DURATION) continue;

            if (wasDown)
            {
                buttonEvent = MouseButtonEvent{
                    KeyButtonState::release,
                    KeyButtonState::release,
                    currentTime,
                };
            }
            else
            {
                buttonEvent = MouseButtonEvent{
                    KeyButtonState::none,
                    buttonEvent.lastSignificantState,
                    buttonEvent.metadata.timestamp,
                    buttonEvent.metadata.duration,
                };
            }
        }
        else if (action == InputAction::press)
        {
            if (utils::hasFlag(buttonEvent.lastSignificantState, KeyButtonState::release) &&
                actionDuration > DOUBLE_CLICK_MIN_INTERVAL &&
                actionDuration < DOUBLE_CLICK_MAX_INTERVAL)
            {
                buttonEvent = MouseButtonEvent{
                    KeyButtonState::press | KeyButtonState::double_press,
                    KeyButtonState::press | KeyButtonState::double_press,
                    currentTime,
                    actionDuration,
                };
            }
            else if (wasDown && actionDuration < KEY_HOLD_MIN_DURATION)
            {
                buttonEvent = MouseButtonEvent{
                    KeyButtonState::hold,
                    KeyButtonState::hold,
                    currentTime,
                    buttonEvent.metadata.duration + currentTime - buttonEvent.metadata.timestamp,
                };
            }
            else
            {
                buttonEvent = MouseButtonEvent{
                    KeyButtonState::press,
                    KeyButtonState::press,
                    currentTime,
                };
            }
        }
    }

    if (!m_rawInputHandler->isActive())
    {
        const auto lastSample = m_cursorPosSamples.back().unwrap();

        MouseCursorPosSample fakeSample{
            glm::vec2(0.f),
            lastSample.pos,
            currentTime,
        };

        m_cursorPosSamples.push(fakeSample);
    }
    else
    {
        const auto& cursorPosInput = m_rawInputHandler->getCursorPosInput();

        m_cursorPosEvent = MouseCursorPosEvent{
            cursorPosInput,
            m_cursorPosEvent.rawPos,
            currentTime,
            m_pollCount,
        };

        const auto lastSample = m_cursorPosSamples.back().unwrap();

        if (currentTime - lastSample.timestamp > 16ms)
        {
            MouseCursorPosSample sample{
                cursorPosInput,
                lastSample.pos,
                currentTime,
            };

            m_cursorPosSamples.push(sample);
        }
    }

    if (!m_rawInputHandler->mouseScrollInputAvailable() || !m_rawInputHandler->isActive())
    {
        const auto lastSample = m_mouseScrollWheelSamples.back();

        MouseWheelScrollSample fakeSample{
            glm::vec2(0.f),
            currentTime,
        };

        m_mouseScrollWheelSamples.push(fakeSample);
    }
    else
    {
        while (m_rawInputHandler->mouseScrollInputAvailable())
        {
            const auto& mouseScrollInput = m_rawInputHandler->getMouseScrollInput();

            m_mouseScrollEvent.metadata.timestamp = currentTime;
            m_mouseScrollEvent.rawScrollOffset = mouseScrollInput;

            const auto lastSample = m_mouseScrollWheelSamples.back().unwrap();

            if (currentTime - lastSample.timestamp > 16ms)
            {
                MouseWheelScrollSample sample{
                    glm::vec2(mouseScrollInput),
                    currentTime,
                };

                m_mouseScrollWheelSamples.push(sample);
            }
        }
    }
}

const glm::vec2 InputArena::getWheelOffset() const { return m_mouseScrollEvent.rawScrollOffset; }

const glm::vec2 InputArena::getAveragedWheelDeltas() const
{
    if (m_mouseScrollWheelSamples.empty()) return {0.0, 0.0};

    double xSum = 0.0;
    double ySum = 0.0;

    for (size_t i = 0; i < m_mouseScrollWheelSamples.size(); ++i)
    {
        xSum += m_mouseScrollWheelSamples[i].rawScrollOffset.x;
        ySum += m_mouseScrollWheelSamples[i].rawScrollOffset.y;
    }

    return {
        xSum / static_cast<double>(m_mouseScrollWheelSamples.size()),
        ySum / static_cast<double>(m_mouseScrollWheelSamples.size()),
    };
}

const glm::vec2 InputArena::getWheelSpeed() const
{
    if (m_mouseScrollWheelSamples.empty()) return {0.0, 0.0};

    auto oldestTime = m_mouseScrollWheelSamples.front().unwrap().timestamp;
    auto newestTime = m_mouseScrollWheelSamples.back().unwrap().timestamp;
    auto timeSpan = std::chrono::duration<float>(newestTime - oldestTime).count();

    if (timeSpan == 0) return {0.0, 0.0};

    return getAveragedWheelDeltas() / timeSpan;
}

const glm::vec2 InputArena::getWheelAccelleration() const
{
    if (m_mouseScrollWheelSamples.empty()) return glm::vec2(0.f);

    auto oldestTime = m_mouseScrollWheelSamples.front().unwrap().timestamp;
    auto newestTime = m_mouseScrollWheelSamples.back().unwrap().timestamp;
    auto timeSpan = std::chrono::duration<float>(newestTime - oldestTime).count();

    if (timeSpan == 0) return glm::vec2(0.f);

    auto speed = getAveragedWheelDeltas() / timeSpan;

    return speed / timeSpan;
}

const glm::vec2 InputArena::getCursorPosition() const { return m_cursorPosEvent.rawPos; }

const glm::vec2 InputArena::getCursorDelta() const
{
    return m_cursorPosEvent.rawPos - m_cursorPosEvent.lastRawPos;
}

const glm::vec2 InputArena::getAveragedCursorDeltas() const
{
    if (m_cursorPosSamples.empty()) return glm::vec2(0.f);

    glm::vec2 sum = glm::vec2(0.f);

    for (size_t i = 0; i < m_cursorPosSamples.size(); ++i)
    {
        sum += m_cursorPosSamples[i].getDelta();
    }

    return sum / static_cast<float>(m_cursorPosSamples.size());
}

const glm::vec2 InputArena::getCursorSpeed() const
{
    if (m_cursorPosSamples.empty()) return glm::vec2(0.f);

    auto oldestTime = m_cursorPosSamples.front().unwrap().timestamp;
    auto newestTime = m_cursorPosSamples.back().unwrap().timestamp;
    auto timeSpan = std::chrono::duration<float>(newestTime - oldestTime).count();

    if (timeSpan == 0) return glm::vec2(0.f);

    return getAveragedCursorDeltas() / timeSpan;
};

const double InputArena::getCursorLinearSpeed() const { return glm::length(getCursorSpeed()); }

const glm::vec2 InputArena::getCursorAccelleration() const
{
    if (m_cursorPosSamples.empty()) return glm::vec2(0.f);

    auto oldestTime = m_cursorPosSamples.front().unwrap().timestamp;
    auto newestTime = m_cursorPosSamples.back().unwrap().timestamp;
    auto timeSpan = std::chrono::duration<float>(newestTime - oldestTime).count();

    if (timeSpan == 0) return glm::vec2(0.f);

    const glm::vec2 speed = getAveragedCursorDeltas() / timeSpan;

    return speed / timeSpan;
}

const double InputArena::getCursorLinearAccelleration() const
{
    return glm::length(getCursorAccelleration());
}

const MovementDirection InputArena::getCursorMovementDirection() const
{
    const glm::vec2 delta = getAveragedCursorDeltas();

    if (glm::length(delta) < 128)
    {
        return MovementDirection::none;
    }

    MovementDirection dir = MovementDirection::none;

    const float absX = std::abs(delta.x);
    const float absY = std::abs(delta.y);

    if (absX > 128)
    {
        if (delta.x < 0)
        {
            dir |= MovementDirection::left;
        }
        else
        {
            dir |= MovementDirection::right;
        }
    }

    if (absY > 128)
    {
        if (delta.y < 0)
        {
            dir |= MovementDirection::up;
        }
        else
        {
            dir |= MovementDirection::down;
        }
    }

    return dir;
}

} // namespace input
} // namespace mosaic
