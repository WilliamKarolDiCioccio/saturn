#include "emscripten_platform.hpp"

namespace mosaic
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

    auto result = m_app->initialize();

    if (result.isErr())
    {
        return pieces::ErrRef<core::Platform, std::string>(std::move(result.error()));
    }

    m_app->resume();

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
    m_app->pause();

    emscripten_pause_main_loop();
}

void EmscriptenPlatform::resume()
{
    m_app->resume();

    emscripten_resume_main_loop();
}

void EmscriptenPlatform::shutdown()
{
    m_app->shutdown();

    glfwTerminate();
}

} // namespace emscripten
} // namespace platform
} // namespace mosaic
