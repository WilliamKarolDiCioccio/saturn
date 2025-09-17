#include "mosaic/input/input_system.hpp"

namespace mosaic
{
namespace input
{

InputSystem* InputSystem::g_instance = nullptr;

pieces::RefResult<core::System, std::string> InputSystem::initialize()
{
    return pieces::OkRef<core::System, std::string>(*this);
}

void InputSystem::shutdown() { unregisterAllWindows(); }

pieces::Result<InputContext*, std::string> InputSystem::registerWindow(window::Window* _window)
{
    if (m_contexts.find(_window) != m_contexts.end())
    {
        MOSAIC_WARN("InputSystem: Window already registered");

        return pieces::Ok<InputContext*, std::string>(m_contexts.at(_window).get());
    }

    m_contexts[_window] = std::make_unique<InputContext>(_window);

    auto result = m_contexts.at(_window)->initialize();

    if (result.isErr())
    {
        m_contexts.erase(_window);

        return pieces::Err<InputContext*, std::string>(std::move(result.error()));
    }

    return pieces::Ok<InputContext*, std::string>(m_contexts.at(_window).get());
}

void InputSystem::unregisterWindow(window::Window* _window)
{
    auto it = m_contexts.find(_window);

    if (it != m_contexts.end())
    {
        it->second->shutdown();

        m_contexts.erase(it);
    }
}

} // namespace input
} // namespace mosaic
