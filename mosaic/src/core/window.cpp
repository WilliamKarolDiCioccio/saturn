#include "mosaic/core/window.hpp"

#if defined(MOSAIC_PLATFORM_DESKTOP) || defined(MOSAIC_PLATFORM_WEB)
#include "platform/GLFW/glfw_window.hpp"
#elif defined(MOSAIC_PLATFORM_ANDROID)
#include "platform/AGDK/agdk_window.hpp"
#endif

namespace mosaic
{
namespace core
{

Window::Window(const std::string& _title, glm::ivec2 _size)
{
    m_properties.title = _title;
    m_properties.size = _size;
}

std::unique_ptr<Window> Window::create(const std::string& _title, glm::ivec2 _size)
{
#if defined(MOSAIC_PLATFORM_DESKTOP) || defined(MOSAIC_PLATFORM_WEB)
    return std::make_unique<platform::glfw::GLFWWindow>(_title, _size);
#elif defined(MOSAIC_PLATFORM_ANDROID)
    return std::make_unique<platform::agdk::AGDKWindow>(_title, _size);
#endif
}

void Window::invokeCloseCallbacks()
{
    for (auto& callback : m_windowCloseCallbacks)
    {
        callback();
    }
}

void Window::invokeFocusCallbacks(int _focused)
{
    for (auto& callback : m_windowFocusCallbacks)
    {
        callback(_focused);
    }
}

void Window::invokeResizeCallbacks(int _width, int _height)
{
    for (auto& callback : m_windowResizeCallbacks)
    {
        callback(_width, _height);
    }
}

void Window::invokeRefreshCallbacks()
{
    for (auto& callback : m_windowRefreshCallbacks)
    {
        callback();
    }
}

void Window::invokeIconifyCallbacks(int _iconified)
{
    for (auto& callback : m_windowIconifyCallbacks)
    {
        callback(_iconified);
    }
}

void Window::invokeMaximizeCallbacks(int _maximized)
{
    for (auto& callback : m_windowMaximizeCallbacks)
    {
        callback(_maximized);
    }
}

void Window::invokeDropCallbacks(int _count, const char** _paths)
{
    for (auto& callback : m_windowDropCallbacks)
    {
        callback(_count, _paths);
    }
}

void Window::invokeScrollCallbacks(double _xoffset, double _yoffset)
{
    for (auto& callback : m_windowScrollCallbacks)
    {
        callback(_xoffset, _yoffset);
    }
}

void Window::invokeCursorEnterCallbacks(int _entered)
{
    for (auto& callback : m_windowCursorEnterCallbacks)
    {
        callback(_entered);
    }
}

void Window::invokePosCallbacks(int _x, int _y)
{
    for (auto& callback : m_windowPosCallbacks)
    {
        callback(_x, _y);
    }
}

void Window::invokeContentScaleCallbacks(float _xscale, float _yscale)
{
    for (auto& callback : m_windowContentScaleCallbacks)
    {
        callback(_xscale, _yscale);
    }
}

} // namespace core
} // namespace mosaic
