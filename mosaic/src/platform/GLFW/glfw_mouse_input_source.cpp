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
      m_cumulativeWheelOffset(0.0f, 0.0f) {};

pieces::RefResult<input::InputSource, std::string> GLFWMouseInputSource::initialize()
{
    m_nativeHandle = static_cast<GLFWwindow*>(m_window->getNativeHandle());

    m_isActive = glfwGetWindowAttrib(m_nativeHandle, GLFW_FOCUSED);

    if (glfwRawMouseMotionSupported())
    {
        glfwSetInputMode(m_nativeHandle, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }

    m_window->registerWindowFocusCallback([this](int _focused)
                                          { m_isActive = _focused == GLFW_TRUE; });

    m_window->registerWindowScrollCallback(
        [this](double xoffset, double yoffset)
        { m_cumulativeWheelOffset += glm::vec2(xoffset, yoffset); });

    return pieces::OkRef<input::InputSource, std::string>(*this);
}

void GLFWMouseInputSource::shutdown() {}

void GLFWMouseInputSource::pollDevice()
{
    m_cumulativeWheelOffset = glm::vec2(0.0f); // Reset wheel offset for the next frame

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

glm::vec2 GLFWMouseInputSource::queryWheelOffset() const { return m_cumulativeWheelOffset; }

} // namespace glfw
} // namespace platform
} // namespace mosaic
