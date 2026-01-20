#include "emscripten_platform.hpp"

namespace saturn
{
namespace platform
{
namespace emscripten
{

pieces::RefResult<core::Platform, std::string> EmscriptenPlatform::initialize()
{
    if (!glfwInit())
    {
        return pieces::ErrRef<core::Platform, std::string>("Failed to initialize GLFW");
    }

    auto result = getApplication()->initialize();

    if (result.isErr())
    {
        return pieces::ErrRef<core::Platform, std::string>(std::move(result.error()));
    }

    getApplication()->resume();

    return pieces::OkRef<core::Platform, std::string>(*this);
}

pieces::RefResult<core::Platform, std::string> EmscriptenPlatform::run()
{
    static std::string errorMsg;

    auto callback = [](void* _arg)
    {
        auto pApp = reinterpret_cast<core::Application*>(_arg);

        glfwPollEvents();

        if (pApp->shouldExit())
        {
            return emscripten_cancel_main_loop();
        }

        if (pApp->isResumed())
        {
            auto result = pApp->update();

            if (result.isErr())
            {
                errorMsg = std::move(result.error());

                return emscripten_cancel_main_loop();
            }
        }
    };

    emscripten_set_main_loop_arg(callback, this, 0, true);

    if (!errorMsg.empty())
    {
        return pieces::ErrRef<core::Platform, std::string>(std::move(errorMsg));
    }

    return pieces::OkRef<core::Platform, std::string>(*this);
}

void EmscriptenPlatform::pause()
{
    getApplication()->pause();

    emscripten_pause_main_loop();
}

void EmscriptenPlatform::resume()
{
    getApplication()->resume();

    emscripten_resume_main_loop();
}

void EmscriptenPlatform::shutdown()
{
    getApplication()->shutdown();

    glfwTerminate();
}

} // namespace emscripten
} // namespace platform
} // namespace saturn
