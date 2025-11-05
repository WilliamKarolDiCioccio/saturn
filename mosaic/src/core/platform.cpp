#include "mosaic/core/platform.hpp"

#include <cassert>
#include <memory>
#include <vector>

#if defined(MOSAIC_PLATFORM_WINDOWS)
#include "platform/Win32/win32_platform.hpp"
#elif defined(MOSAIC_PLATFORM_EMSCRIPTEN)
#include "platform/emscripten/emscripten_platform.hpp"
#elif defined(MOSAIC_PLATFORM_ANDROID)
#include "mosaic/platform/AGDK/agdk_platform.hpp"
#endif

#include "mosaic/defines.hpp"
#include "mosaic/core/application.hpp"

namespace mosaic
{
namespace core
{

struct PlatformContext::Impl
{
    std::vector<PlatformContextChangedEvent> platformContextListeners;
};

inline void PlatformContext::registerPlatformContextChangedCallback(
    PlatformContextChangedEvent _callback)
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
#if defined(MOSAIC_PLATFORM_ANDROID)
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
#if defined(MOSAIC_PLATFORM_WINDOWS)
    return std::make_unique<platform::win32::Win32Platform>(_app);
#elif defined(MOSAIC_PLATFORM_EMSCRIPTEN)
    return std::make_unique<platform::emscripten::EmscriptenPlatform>(_app);
#elif defined(MOSAIC_PLATFORM_ANDROID)
    return std::make_unique<platform::agdk::AGDKPlatform>(_app);
#endif
}

PlatformContext* Platform::getPlatformContext() { return m_impl->platformContext.get(); }

Application* Platform::getApplication() { return m_impl->app; }

} // namespace core
} // namespace mosaic
