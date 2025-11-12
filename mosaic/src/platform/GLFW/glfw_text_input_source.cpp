#include "glfw_text_input_source.hpp"

namespace mosaic
{
namespace platform
{
namespace glfw
{

GLFWTextInputSource::GLFWTextInputSource(window::Window* window)
    : input::TextInputSource(window), m_nativeHandle(nullptr), m_charCallbackId(0){};

pieces::RefResult<input::InputSource, std::string> GLFWTextInputSource::initialize()
{
    m_nativeHandle = static_cast<GLFWwindow*>(m_window->getNativeHandle());

    m_isActive = glfwGetWindowAttrib(static_cast<GLFWwindow*>(m_nativeHandle), GLFW_FOCUSED);

    m_charCallbackId = m_window->registerWindowCharCallback(
        [this](unsigned int codepoint)
        {
            if (!this) return;

            m_codepointsBuffer.emplace_back(static_cast<char32_t>(codepoint));
        });

    m_focusCallbackId = m_window->registerWindowFocusCallback(
        [this](int _focused) { m_isActive = _focused == GLFW_TRUE; });

    return pieces::OkRef<input::InputSource, std::string>(*this);
}

void GLFWTextInputSource::shutdown()
{
    m_window->unregisterWindowCharCallback(m_charCallbackId);
    m_window->unregisterWindowFocusCallback(m_focusCallbackId);
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
