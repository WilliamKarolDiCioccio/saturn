#include "saturn/core/platform.hpp"

#include <cassert>
#include <memory>
#include <vector>

#if defined(SATURN_PLATFORM_WINDOWS)
#include "platform/Win32/win32_platform.hpp"
#elif defined(SATURN_PLATFORM_LINUX)
#include "platform/POSIX/posix_platform.hpp"
#elif defined(SATURN_PLATFORM_EMSCRIPTEN)
#include "platform/emscripten/emscripten_platform.hpp"
#elif defined(SATURN_PLATFORM_ANDROID)
#include "saturn/platform/AGDK/agdk_platform.hpp"
#endif

#include "saturn/defines.hpp"
#include "saturn/core/application.hpp"

namespace saturn
{
namespace core
{

struct PlatformContext::Impl
{
    std::vector<PlatformContextChangedEvent> platformContextListeners;
};

void PlatformContext::registerPlatformContextChangedCallback(PlatformContextChangedEvent _callback)
{
    m_impl->platformContextListeners.push_back(_callback);
}

void PlatformContext::invokePlatformContextChangedCallbacks(void* _newContext)
{
    for (const auto& listener : m_impl->platformContextListeners) listener(_newContext);
}

struct Platform::Impl
{
    Application* app;
    std::unique_ptr<PlatformContext> platformContext;
    std::vector<PlatformContextChangedEvent> m_platformContextListeners;

    Impl(Application* _app) : app(_app), platformContext(PlatformContext::create()) {};
};

std::unique_ptr<PlatformContext> PlatformContext::create()
{
#if defined(SATURN_PLATFORM_ANDROID)
    return std::make_unique<platform::agdk::AGDKPlatformContext>();
#else
    return nullptr; // No platform context for non-Android platforms (at least for now)
#endif
}

Platform* Platform::s_instance = nullptr;

Platform::Platform(Application* _app) : m_impl(new Impl(_app))
{
    assert(!s_instance && "Platform instance already exists!");
    s_instance = this;
}

Platform::~Platform()
{
    assert(s_instance == this);
    s_instance = nullptr;
    delete m_impl;
}

std::unique_ptr<Platform> Platform::create(Application* _app)
{
#if defined(SATURN_PLATFORM_WINDOWS)
    return std::make_unique<platform::win32::Win32Platform>(_app);
#elif defined(SATURN_PLATFORM_LINUX)
    return std::make_unique<platform::posix::POSIXPlatform>(_app);
#elif defined(SATURN_PLATFORM_EMSCRIPTEN)
    return std::make_unique<platform::emscripten::EmscriptenPlatform>(_app);
#elif defined(SATURN_PLATFORM_ANDROID)
    return std::make_unique<platform::agdk::AGDKPlatform>(_app);
#endif
}

PlatformContext* Platform::getPlatformContext() { return m_impl->platformContext.get(); }

Application* Platform::getApplication() { return m_impl->app; }

} // namespace core
} // namespace saturn
