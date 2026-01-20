#include "saturn/input/sources/keyboard_input_source.hpp"

#if defined(SATURN_PLATFORM_DESKTOP) || defined(SATURN_PLATFORM_WEB)
#include "platform/GLFW/glfw_keyboard_input_source.hpp"
#endif

using namespace std::chrono_literals;

namespace saturn
{
namespace input
{

KeyboardInputSource::KeyboardInputSource(window::Window* _window) : InputSource(_window)
{
    auto currentTime = std::chrono::high_resolution_clock::now();

    KeyboardKeyEvent dummyKeyEvent = {
        ActionableState::none,
        currentTime,
        m_pollCount,
        0ms,
    };

    for (auto& buffer : m_keyboardKeyEvents)
    {
        buffer.push(dummyKeyEvent);
    }
};

std::unique_ptr<KeyboardInputSource> KeyboardInputSource::create(window::Window* _window)
{
#if defined(SATURN_PLATFORM_DESKTOP) || defined(SATURN_PLATFORM_WEB)
    return std::make_unique<platform::glfw::GLFWKeyboardInputSource>(_window);
#else
    throw std::runtime_error("Keyboard input source not supported on mobile devices");
#endif
}

void KeyboardInputSource::processInput()
{
    if (!isActive()) return;

    pollDevice();

    auto currentTime = std::chrono::high_resolution_clock::now();

    for (const auto& key : c_keyboardKeys)
    {
        const InputAction action = queryKeyState(key);
        auto& eventQueue = m_keyboardKeyEvents[static_cast<uint32_t>(key)];

        const auto lastEvent = eventQueue.front().unwrap();

        const bool wasDown = utils::hasFlag(lastEvent.state, ActionableState::press) ||
                             utils::hasFlag(lastEvent.state, ActionableState::hold);

        auto timeSinceLastEvent = std::chrono::duration_cast<std::chrono::milliseconds>(
            currentTime - lastEvent.metadata.timestamp);

        KeyboardKeyEvent currEvent;

        if (action == InputAction::press)
        {
            if (wasDown)
            {
                if (utils::hasFlag(lastEvent.state, ActionableState::press) &&
                    timeSinceLastEvent >= k_keyHoldMinDuration)
                {
                    currEvent = KeyboardKeyEvent{
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
                        currentTime - lastEvent.metadata.timestamp);

                    currEvent = KeyboardKeyEvent{
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
                    currEvent = KeyboardKeyEvent{
                        ActionableState::press | ActionableState::double_press,
                        currentTime,
                        m_pollCount,
                        std::chrono::milliseconds(0), // Press is instantaneous
                    };
                }
                else
                {
                    currEvent = KeyboardKeyEvent{
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
                    currEvent = KeyboardKeyEvent{
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
}

} // namespace input
} // namespace saturn
