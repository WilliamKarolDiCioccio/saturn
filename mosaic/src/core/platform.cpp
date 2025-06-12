#include "mosaic/core/platform.hpp"

#if defined(MOSAIC_PLATFORM_WINDOWS)
#include "platform/Win32/win32_platform.hpp"
#elif defined(MOSAIC_PLATFORM_EMSCRIPTEN)
#include "platform/emscripten/emscripten_platform.hpp"
#elif defined(MOSAIC_PLATFORM_ANDROID)
#include "mosaic/platform/AGDK/agdk_platform.hpp"
#endif

namespace mosaic
{
namespace core
{

std::unique_ptr<PlatformContext> PlatformContext::create()
{
#if defined(MOSAIC_PLATFORM_ANDROID)
    return std::make_unique<platform::agdk::AGDKPlatformContext>();
#else
    return nullptr; // No platform context for non-Android platforms (at least for now)
#endif
}

void PlatformContext::invokePlatformContextChangedCallbacks(void* _newContext)
{
    for (const auto& listener : m_platformContextListeners)
    {
        listener(_newContext);
    }
}

Platform* Platform::s_instance = nullptr;

Platform::Platform(Application* _app) : m_app(_app), m_platformContext(PlatformContext::create())
{
    assert(!s_instance && "Platform instance already exists!");

    s_instance = this;
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

} // namespace core
} // namespace mosaic
