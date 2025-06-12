#include "glfw_raw_input_handler.hpp"

namespace mosaic
{
namespace platform
{
namespace glfw
{

GLFWRawInputHandler::GLFWRawInputHandler(const core::Window* _window)
    : m_window(const_cast<core::Window*>(_window)), input::RawInputHandler(_window) {};

pieces::RefResult<input::RawInputHandler, std::string> GLFWRawInputHandler::initialize()
{
    m_isActive = glfwGetWindowAttrib(static_cast<GLFWwindow*>(m_nativeHandle), GLFW_FOCUSED);

    registerCallbacks();

    return pieces::OkRef<input::RawInputHandler, std::string>(*this);
}

void GLFWRawInputHandler::shutdown() { m_isActive = false; }

input::RawInputHandler::KeyboardKeyInputData GLFWRawInputHandler::getKeyboardKeyInput(
    input::KeyboardKey _key) const
{
    return static_cast<input::RawInputHandler::KeyboardKeyInputData>(
        glfwGetKey(static_cast<GLFWwindow*>(m_nativeHandle), static_cast<int>(_key)));
}

input::RawInputHandler::MouseButtonInputData GLFWRawInputHandler::getMouseButtonInput(
    input::MouseButton _button) const
{
    return static_cast<input::RawInputHandler::MouseButtonInputData>(
        glfwGetMouseButton(static_cast<GLFWwindow*>(m_nativeHandle), static_cast<int>(_button)));
}

input::RawInputHandler::CursorPosInputData GLFWRawInputHandler::getCursorPosInput()
{
    double xpos, ypos;
    glfwGetCursorPos(static_cast<GLFWwindow*>(m_nativeHandle), &xpos, &ypos);
    return glm::vec2(xpos, ypos);
}

void GLFWRawInputHandler::registerCallbacks()
{
    m_window->registerWindowFocusCallback(
        [this](int focused)
        {
            if (!this)
            {
                MOSAIC_ERROR("GLFWRawInputHandler: No handler found for window focus callback");
                return;
            }

            this->setActive(focused == GLFW_TRUE);
        });

    m_window->registerWindowScrollCallback(
        [this](double xoffset, double yoffset)
        {
            if (!this)
            {
                MOSAIC_ERROR("GLFWRawInputHandler: No handler found for window scroll callback");
                return;
            }

            this->addMouseScrollInput(xoffset, yoffset);
        });
}

} // namespace glfw
} // namespace platform
} // namespace mosaic
