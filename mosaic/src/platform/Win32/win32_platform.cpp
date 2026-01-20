#include "win32_platform.hpp"

#include <GLFW/glfw3.h>

#pragma comment(lib, "ole32.lib")

namespace saturn
{
namespace platform
{
namespace win32
{

pieces::RefResult<core::Platform, std::string> Win32Platform::initialize()
{
    HRESULT hres = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    if (FAILED(hres))
    {
        return pieces::ErrRef<core::Platform, std::string>("Failed to initialize COM library.");
    }

    hres = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT,
                                RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
    if (FAILED(hres))
    {
        CoUninitialize();

        return pieces::ErrRef<core::Platform, std::string>("Failed to initialize COM security.");
    }

    auto app = getApplication();

    auto result = app->initialize();

    if (result.isErr())
    {
        return pieces::ErrRef<core::Platform, std::string>(std::move(result.error()));
    }

    app->resume();

    return pieces::OkRef<Platform, std::string>(*this);
}

pieces::RefResult<core::Platform, std::string> Win32Platform::run()
{
    auto app = getApplication();

    while (!app->shouldExit())
    {
        if (app->isResumed())
        {
            auto result = app->update();

            if (result.isErr())
            {
                return pieces::ErrRef<core::Platform, std::string>(std::move(result.error()));
            }
        }
    }

    return pieces::OkRef<core::Platform, std::string>(*this);
}

void Win32Platform::pause() { getApplication()->pause(); }

void Win32Platform::resume() { getApplication()->resume(); }

void Win32Platform::shutdown()
{
    getApplication()->shutdown();

    CoUninitialize();
}

} // namespace win32
} // namespace platform
} // namespace saturn
