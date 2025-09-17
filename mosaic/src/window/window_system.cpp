#include "mosaic/window/window_system.hpp"

#if defined(MOSAIC_PLATFORM_DESKTOP) || defined(MOSAIC_PLATFORM_WEB)
#include "platform/GLFW/glfw_window_system.hpp"
#elif defined(MOSAIC_PLATFORM_ANDROID)
#include "platform/AGDK/agdk_window_system.hpp"
#endif

namespace mosaic
{
namespace window
{

WindowSystem* WindowSystem::g_instance = nullptr;

std::unique_ptr<WindowSystem> WindowSystem::create()
{
#if defined(MOSAIC_PLATFORM_DESKTOP) || defined(MOSAIC_PLATFORM_WEB)
    return std::make_unique<platform::glfw::GLFWWindowSystem>();
#elif defined(MOSAIC_PLATFORM_ANDROID)
    return std::make_unique<platform::agdk::AGDKWindowSystem>();
#endif
}

pieces::Result<Window*, std::string> WindowSystem::createWindow(const std::string& _windowId,
                                                                const WindowProperties& _properties)
{
    if (m_windows.find(_windowId) != m_windows.end())
    {
        MOSAIC_WARN("InputSystem: Window already registered");

        return pieces::Ok<Window*, std::string>(m_windows.at(_windowId).get());
    }

    m_windows[_windowId] = Window::create();

    auto result = m_windows.at(_windowId)->initialize(_properties);

    if (result.isErr())
    {
        m_windows.erase(_windowId);

        return pieces::Err<Window*, std::string>(std::move(result.error()));
    }

    return pieces::Ok<Window*, std::string>(m_windows.at(_windowId).get());
}

void WindowSystem::destroyWindow(const std::string& _windowId)
{
    auto it = m_windows.find(_windowId);

    if (it != m_windows.end())
    {
        it->second->shutdown();

        m_windows.erase(it);
    }
}

} // namespace window
} // namespace mosaic
