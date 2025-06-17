#include "glfw_mouse_input_source.hpp"

namespace mosaic
{
namespace platform
{
namespace glfw
{

GLFWMouseInputSource::GLFWMouseInputSource(window::Window* _window)
    : input::MouseInputSource(_window),
      m_nativeHandle(nullptr),
      m_cumulativeWheelOffse(0.0f, 0.0f) {};

pieces::RefResult<input::InputSource, std::string> GLFWMouseInputSource::initialize()
{
    m_nativeHandle = static_cast<GLFWwindow*>(m_window->getNativeHandle());

    m_isActive = glfwGetWindowAttrib(m_nativeHandle, GLFW_FOCUSED);

    m_window->registerWindowFocusCallback([this](int _focused)
                                          { m_isActive = _focused == GLFW_TRUE; });

    m_window->registerWindowScrollCallback(
        [this](double xoffset, double yoffset)
        { m_cumulativeWheelOffse += glm::vec2(xoffset, yoffset); });

    return pieces::OkRef<input::InputSource, std::string>(*this);
}

void GLFWMouseInputSource::shutdown() {}

void GLFWMouseInputSource::pollDevice()
{
    // Already polled by the window system
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

glm::vec2 GLFWMouseInputSource::queryWheelOffset() const { return m_cumulativeWheelOffse; }

} // namespace glfw
} // namespace platform
} // namespace mosaic
