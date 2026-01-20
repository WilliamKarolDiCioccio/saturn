#include "glfw_keyboard_input_source.hpp"

namespace saturn
{
namespace platform
{
namespace glfw
{

GLFWKeyboardInputSource::GLFWKeyboardInputSource(window::Window* _window)
    : input::KeyboardInputSource(_window), m_nativeHandle(nullptr), m_focusCallbackId(0){};

pieces::RefResult<input::InputSource, std::string> GLFWKeyboardInputSource::initialize()
{
    m_nativeHandle = static_cast<GLFWwindow*>(m_window->getNativeHandle());

    m_isActive = glfwGetWindowAttrib(static_cast<GLFWwindow*>(m_nativeHandle), GLFW_FOCUSED);

    m_focusCallbackId = m_window->registerWindowFocusCallback(
        [this](int _focused) { m_isActive = _focused == GLFW_TRUE; });

    return pieces::OkRef<InputSource, std::string>(*this);
}

void GLFWKeyboardInputSource::shutdown()
{
    m_window->unregisterWindowFocusCallback(m_focusCallbackId);
}

void GLFWKeyboardInputSource::pollDevice()
{
    ++m_pollCount;

    // Already polled by the window system
}

input::InputAction GLFWKeyboardInputSource::queryKeyState(input::KeyboardKey _key) const
{
    auto state = glfwGetKey(m_nativeHandle, static_cast<int>(_key));
    return static_cast<input::InputAction>(state);
}

} // namespace glfw
} // namespace platform
} // namespace saturn
