#include "saturn/input/input_system.hpp"

#include <cassert>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include <pieces/core/result.hpp>

#include "saturn/tools/logger.hpp"
#include "saturn/core/system.hpp"
#include "saturn/input/input_context.hpp"
#include "saturn/window/window.hpp"

namespace saturn
{
namespace input
{

struct InputSystem::Impl
{
    std::unordered_map<const window::Window*, std::unique_ptr<InputContext>> contexts;

    Impl() = default;
};

InputSystem* InputSystem::g_instance = nullptr;

InputSystem::InputSystem() : EngineSystem(core::EngineSystemType::input), m_impl(new Impl())
{
    assert(!g_instance && "InputSystem already exists!");
    g_instance = this;
};

InputSystem::~InputSystem()
{
    g_instance = nullptr;
    delete m_impl;
}

pieces::RefResult<core::System, std::string> InputSystem::initialize()
{
    return pieces::OkRef<core::System, std::string>(*this);
}

void InputSystem::shutdown() { unregisterAllWindows(); }

pieces::Result<InputContext*, std::string> InputSystem::registerWindow(window::Window* _window)
{
    auto& contexts = m_impl->contexts;

    if (contexts.find(_window) != contexts.end())
    {
        SATURN_WARN("InputSystem: Window already registered");

        return pieces::Ok<InputContext*, std::string>(contexts.at(_window).get());
    }

    contexts[_window] = std::make_unique<InputContext>(_window);

    auto result = contexts.at(_window)->initialize();

    if (result.isErr())
    {
        contexts.erase(_window);

        return pieces::Err<InputContext*, std::string>(std::move(result.error()));
    }

    return pieces::Ok<InputContext*, std::string>(contexts.at(_window).get());
}

void InputSystem::unregisterWindow(window::Window* _window)
{
    auto& contexts = m_impl->contexts;

    auto it = contexts.find(_window);

    if (it != contexts.end())
    {
        it->second->shutdown();

        contexts.erase(it);
    }
}

void InputSystem::unregisterAllWindows()
{
    for (auto& [window, context] : m_impl->contexts) context->shutdown();

    m_impl->contexts.clear();
}

pieces::RefResult<core::System, std::string> InputSystem::update()
{
    for (auto& [window, context] : m_impl->contexts) context->update();

    return pieces::OkRef<System, std::string>(*this);
}

InputContext* InputSystem::getContext(const window::Window* _window) const
{
    auto& contexts = m_impl->contexts;

    if (contexts.find(_window) != contexts.end()) return contexts.at(_window).get();

    return nullptr;
}

} // namespace input
} // namespace saturn
