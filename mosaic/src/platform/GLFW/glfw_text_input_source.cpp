#include "glfw_text_input_source.hpp"

namespace mosaic
{
namespace platform
{
namespace glfw
{

GLFWTextInputSource::GLFWTextInputSource(window::Window* window)
    : input::TextInputSource(window), m_nativeHandle(nullptr) {};

pieces::RefResult<input::InputSource, std::string> GLFWTextInputSource::initialize()
{
    m_nativeHandle = static_cast<GLFWwindow*>(m_window->getNativeHandle());

    m_isActive = glfwGetWindowAttrib(static_cast<GLFWwindow*>(m_nativeHandle), GLFW_FOCUSED);

    m_charCallbackId = m_window->registerWindowCallback<window::WindowCharCallback>(
        [this](unsigned int codepoint)
        {
            if (!this) return;

            m_codepointsBuffer.emplace_back(static_cast<char32_t>(codepoint));
        });

    m_focusCallbackId = m_window->registerWindowCallback<window::WindowFocusCallback>(
        [this](int _focused) { m_isActive = _focused == GLFW_TRUE; });

    return pieces::OkRef<input::InputSource, std::string>(*this);
}

void GLFWTextInputSource::shutdown()
{
    m_window->unregisterWindowCallback<window::WindowCharCallback>(m_charCallbackId);
    m_window->unregisterWindowCallback<window::WindowFocusCallback>(m_focusCallbackId);
}

void GLFWTextInputSource::pollDevice()
{
    ++m_pollCount;

    // Already polled by the window system
}

std::vector<char32_t> GLFWTextInputSource::queryCodepoints()
{
    auto codepoints = m_codepointsBuffer;
    m_codepointsBuffer.clear();
    return codepoints;
}

} // namespace glfw
} // namespace platform
} // namespace mosaic
