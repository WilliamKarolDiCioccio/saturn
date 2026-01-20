#include "saturn/window/window.hpp"

#include <vector>
#include <memory>
#include <atomic>

#include "saturn/defines.hpp"

#if defined(SATURN_PLATFORM_DESKTOP) || defined(SATURN_PLATFORM_WEB)
#include "platform/GLFW/glfw_window.hpp"
#elif defined(SATURN_PLATFORM_ANDROID)
#include "platform/AGDK/agdk_window.hpp"
#endif

namespace saturn
{
namespace window
{

struct Window::Impl
{
    WindowProperties properties;

#define DEFINE_CALLBACK(_Type, _Name, ...) std::vector<_Type> _Name##Callbacks;
#include "saturn/window/callbacks.def"
#undef DEFINE_CALLBACK
};

Window::Window() : m_impl(new Impl()) {}

Window::~Window() { delete m_impl; }

std::unique_ptr<Window> Window::create()
{
#if defined(SATURN_PLATFORM_DESKTOP) || defined(SATURN_PLATFORM_WEB)
    return std::make_unique<platform::glfw::GLFWWindow>();
#elif defined(SATURN_PLATFORM_ANDROID)
    return std::make_unique<platform::agdk::AGDKWindow>();
#endif
}

const WindowProperties Window::getWindowProperties() const { return m_impl->properties; }

const CursorProperties Window::getCursorProperties() const
{
    return m_impl->properties.cursorProperties;
}

WindowProperties& Window::getWindowPropertiesInternal() { return m_impl->properties; }

CursorProperties& Window::getCursorPropertiesInternal()
{
    return m_impl->properties.cursorProperties;
}

#define DEFINE_CALLBACK(_Type, _Name, _Params, _Args)                                       \
    size_t Window::register##_Name##Callback(const _Type::CallbackFn& cb)                   \
    {                                                                                       \
        static std::atomic<size_t> idCounter{0};                                            \
        size_t id = ++idCounter;                                                            \
        m_impl->_Name##Callbacks.emplace_back(id, cb);                                      \
        return id;                                                                          \
    }                                                                                       \
                                                                                            \
    void Window::unregister##_Name##Callback(size_t id)                                     \
    {                                                                                       \
        auto& callbacks = m_impl->_Name##Callbacks;                                         \
        auto it = std::remove_if(callbacks.begin(), callbacks.end(),                        \
                                 [id](const _Type& cb) { return cb.id == id; });            \
        if (it != callbacks.end()) callbacks.erase(it);                                     \
    }                                                                                       \
                                                                                            \
    std::vector<_Type> Window::get##_Name##Callbacks() { return m_impl->_Name##Callbacks; } \
                                                                                            \
    void Window::invoke##_Name##Callbacks _Params                                           \
    {                                                                                       \
        for (auto& cb : m_impl->_Name##Callbacks) cb.callback _Args;                        \
    }

#include "saturn/window/callbacks.def"

#undef DEFINE_CALLBACK

} // namespace window
} // namespace saturn
