#include "saturn/window/window_system.hpp"

#include <cassert>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include <pieces/core/result.hpp>

#include "saturn/tools/logger.hpp"
#include "saturn/core/system.hpp"
#include "saturn/defines.hpp"
#include "saturn/window/window.hpp"

#if defined(SATURN_PLATFORM_DESKTOP) || defined(SATURN_PLATFORM_WEB)
#include "platform/GLFW/glfw_window_system.hpp"
#elif defined(SATURN_PLATFORM_ANDROID)
#include "platform/AGDK/agdk_window_system.hpp"
#endif

namespace saturn
{
namespace window
{

struct WindowSystem::Impl
{
    std::unordered_map<std::string, std::unique_ptr<Window>> windows;

    Impl() = default;
};

WindowSystem* WindowSystem::g_instance = nullptr;

WindowSystem::WindowSystem() : EngineSystem(core::EngineSystemType::window), m_impl(new Impl())
{
    assert(!g_instance && "WindowSystem already exists!");
    g_instance = this;
};

WindowSystem ::~WindowSystem()
{
    g_instance = nullptr;
    delete m_impl;
}

std::unique_ptr<WindowSystem> WindowSystem::create()
{
#if defined(SATURN_PLATFORM_DESKTOP) || defined(SATURN_PLATFORM_WEB)
    return std::make_unique<platform::glfw::GLFWWindowSystem>();
#elif defined(SATURN_PLATFORM_ANDROID)
    return std::make_unique<platform::agdk::AGDKWindowSystem>();
#endif
}

pieces::Result<Window*, std::string> WindowSystem::createWindow(const std::string& _windowId,
                                                                const WindowProperties& _properties)
{
    auto& windows = m_impl->windows;

    if (windows.find(_windowId) != windows.end())
    {
        SATURN_WARN("InputSystem: Window already registered");

        return pieces::Ok<Window*, std::string>(windows.at(_windowId).get());
    }

    windows[_windowId] = Window::create();

    auto result = windows.at(_windowId)->initialize(_properties);

    if (result.isErr())
    {
        windows.erase(_windowId);

        return pieces::Err<Window*, std::string>(std::move(result.error()));
    }

    return pieces::Ok<Window*, std::string>(windows.at(_windowId).get());
}

void WindowSystem::destroyWindow(const std::string& _windowId)
{
    auto& windows = m_impl->windows;

    auto it = windows.find(_windowId);

    if (it != windows.end())
    {
        it->second->shutdown();

        windows.erase(it);
    }
}

void WindowSystem::destroyAllWindows()
{
    for (auto& [window, context] : m_impl->windows) context->shutdown();

    m_impl->windows.clear();
}

[[nodiscard]] Window* WindowSystem::getWindow(const std::string& _windowId) const
{
    auto& windows = m_impl->windows;

    if (windows.find(_windowId) != windows.end()) return windows.at(_windowId).get();

    return nullptr;
}

[[nodiscard]] size_t WindowSystem::getWindowCount() const { return m_impl->windows.size(); }

} // namespace window
} // namespace saturn
