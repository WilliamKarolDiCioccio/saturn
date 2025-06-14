#include "win32_platform.hpp"

#include <GLFW/glfw3.h>
#include <VersionHelpers.h>

#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "Ole32.lib")

namespace mosaic
{
namespace platform
{
namespace win32
{

pieces::RefResult<core::Platform, std::string> Win32Platform::initialize()
{
    auto result = m_app->initialize();

    if (result.isErr())
    {
        return pieces::ErrRef<core::Platform, std::string>(std::move(result.error()));
    }

    m_app->resume();

    return pieces::OkRef<Platform, std::string>(*this);
}

pieces::RefResult<core::Platform, std::string> Win32Platform::run()
{
    while (!m_app->shouldExit())
    {
        if (m_app->isResumed())
        {
            auto result = m_app->update();

            if (result.isErr())
            {
                return pieces::ErrRef<core::Platform, std::string>(std::move(result.error()));
            }
        }
    }

    return pieces::OkRef<core::Platform, std::string>(*this);
}

void Win32Platform::pause() { m_app->pause(); }

void Win32Platform::resume() { m_app->resume(); }

void Win32Platform::shutdown() { m_app->shutdown(); }

} // namespace win32
} // namespace platform
} // namespace mosaic
