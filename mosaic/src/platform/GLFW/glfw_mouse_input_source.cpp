#include "glfw_mouse_input_source.hpp"

namespace saturn
{
namespace platform
{
namespace glfw
{

GLFWMouseInputSource::GLFWMouseInputSource(window::Window* _window)
    : input::MouseInputSource(_window),
      m_nativeHandle(nullptr),
      m_focusCallbackId(0),
      m_wheelOffsetBuffer(0.0f, 0.0f){};

pieces::RefResult<input::InputSource, std::string> GLFWMouseInputSource::initialize()
{
    m_nativeHandle = static_cast<GLFWwindow*>(m_window->getNativeHandle());

    m_isActive = glfwGetWindowAttrib(m_nativeHandle, GLFW_FOCUSED);

    if (glfwRawMouseMotionSupported())
    {
        glfwSetInputMode(m_nativeHandle, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }

    m_focusCallbackId = m_window->registerWindowFocusCallback(
        [this](int _focused) { m_isActive = _focused == GLFW_TRUE; });

    m_scrollCallbackId = m_window->registerWindowScrollCallback(
        [this](double xoffset, double yoffset)
        { m_wheelOffsetBuffer += glm::vec2(xoffset, yoffset); });

    return pieces::OkRef<input::InputSource, std::string>(*this);
}

void GLFWMouseInputSource::shutdown()
{
    m_window->unregisterWindowFocusCallback(m_focusCallbackId);
    m_window->unregisterWindowScrollCallback(m_scrollCallbackId);
}

void GLFWMouseInputSource::pollDevice()
{
    ++m_pollCount;

    // glfwPollEvents() is already called by the window manager, so we don't need to call it here.
}

input::InputAction GLFWMouseInputSource::queryButtonState(input::MouseButton _button) const
{
    auto state = glfwGetMouseButton(m_nativeHandle, static_cast<int>(_button));
    return static_cast<input::InputAction>(state);
}

glm::vec2 GLFWMouseInputSource::queryCursorPosition() const
{
    double xpos, ypos;
    glfwGetCursorPos(m_nativeHandle, &xpos, &ypos);
    return glm::vec2(xpos, ypos);
}

glm::vec2 GLFWMouseInputSource::queryWheelOffset()
{
    auto wheelOffset = m_wheelOffsetBuffer;
    m_wheelOffsetBuffer = glm::vec2(0.0f);
    return wheelOffset;
}

} // namespace glfw
} // namespace platform
} // namespace saturn
