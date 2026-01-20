#include "posix_platform.hpp"

#include <GLFW/glfw3.h>

#pragma comment(lib, "ole32.lib")

namespace saturn
{
namespace platform
{
namespace posix
{

pieces::RefResult<core::Platform, std::string> POSIXPlatform::initialize()
{
    auto app = getApplication();

    auto result = app->initialize();

    if (result.isErr())
    {
        return pieces::ErrRef<core::Platform, std::string>(std::move(result.error()));
    }

    app->resume();

    return pieces::OkRef<Platform, std::string>(*this);
}

pieces::RefResult<core::Platform, std::string> POSIXPlatform::run()
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

void POSIXPlatform::pause() { getApplication()->pause(); }

void POSIXPlatform::resume() { getApplication()->resume(); }

void POSIXPlatform::shutdown() { getApplication()->shutdown(); }

} // namespace posix
} // namespace platform
} // namespace saturn
