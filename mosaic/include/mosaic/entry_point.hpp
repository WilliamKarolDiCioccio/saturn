#pragma once

#include <memory>

#include <mosaic/core/sys_ui.hpp>
#include <mosaic/core/sys_console.hpp>
#include <mosaic/tools/logger.hpp>
#include <mosaic/tools/logger_default_sink.hpp>
#include <mosaic/tools/tracer.hpp>
#include <mosaic/core/cmd_line_parser.hpp>
#include <mosaic/core/platform.hpp>
#include <mosaic/core/application.hpp>

#ifndef MOSAIC_PLATFORM_ANDROID

namespace mosaic
{

template <typename AppType, typename... Args>
    requires core::IsApplication<AppType>
int runApp(const std::vector<std::string>& _cmdLineArgs, Args&&... _appConstuctorArgs)
{
    core::SystemConsole::attachParent();

    core::CommandLineParser::initialize();

    auto cmdLineParser = core::CommandLineParser::getInstance();

    auto parseResult = cmdLineParser->parseCommandLine(_cmdLineArgs);

    if (parseResult.has_value())
    {
        core::SystemConsole::print(cmdLineParser->getHelpText());
        return 1;
    }

    if (cmdLineParser->shouldTerminate()) return 0;

    core::SystemConsole::detachParent();

    core::SystemConsole::create();

    tools::Logger::initialize();

    auto logger = tools::Logger::getInstance();

    logger->addSink<core::DefaultSink>("default", core::DefaultSink());

    tools::Tracer::initialize();

    // This scope guard ensures all resources have been disposed before shutting down the logger and
    // tracer.

    {
        auto app = std::make_unique<AppType>(std::forward<Args>(_appConstuctorArgs)...);

        auto platform = core::Platform::create(app.get());

        auto result = platform->initialize().andThen(std::mem_fn(&core::Platform::run));

        if (result.isErr())
        {
            MOSAIC_ERROR(result.error().c_str());
            core::SystemConsole::destroy();
            return 1;
        }

        platform->shutdown();
    }

    tools::Tracer::shutdown();
    tools::Logger::shutdown();
    core::CommandLineParser::shutdown();

    core::SystemConsole::destroy();

    return 0;
}

} // namespace mosaic

#endif

#if defined(MOSAIC_PLATFORM_WINDOWS)

#include <windows.h>

#include <mosaic/platform/Win32/wstring.hpp>

#define MOSAIC_ENTRY_POINT(AppType, ...)                                            \
    int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,  \
                       _In_ PSTR lpCmdLine, _In_ int nCmdShow)                      \
    {                                                                               \
        int argc = 0;                                                               \
        LPWSTR* argv_w = CommandLineToArgvW(GetCommandLineW(), &argc);              \
                                                                                    \
        if (argv_w == nullptr)                                                      \
        {                                                                           \
            mosaic::core::SystemUI::showErrorDialog("Error parsing cmd line args!", \
                                                    "argv_w is nullptr.");          \
            return 1;                                                               \
        }                                                                           \
                                                                                    \
        std::vector<std::string> args;                                              \
        args.reserve(argc);                                                         \
        for (int i = 0; i < argc; ++i)                                              \
        {                                                                           \
            args.emplace_back(mosaic::platform::win32::WStringToString(argv_w[i])); \
        }                                                                           \
                                                                                    \
        LocalFree(argv_w);                                                          \
                                                                                    \
        return mosaic::runApp<AppType>(args __VA_OPT__(, __VA_ARGS__));             \
    }

#elif defined(MOSAIC_PLATFORM_LINUX) || defined(MOSAIC_PLATFORM_MACOS) || \
    defined(MOSAIC_PLATFORM_EMSCRIPTEN)

#define MOSAIC_ENTRY_POINT(AppType, ...)                                \
    int main(int _argc, char** _argv)                                   \
    {                                                                   \
        std::vector<std::string> args;                                  \
        args.reserve(_argc);                                            \
        for (int i = 0; i < _argc; ++i)                                 \
        {                                                               \
            args.emplace_back(_argv[i]);                                \
        }                                                               \
                                                                        \
        return mosaic::runApp<AppType>(args __VA_OPT__(, __VA_ARGS__)); \
    }

#elif defined(MOSAIC_PLATFORM_ANDROID)

#include <mosaic/platform/AGDK/jni_helper.hpp>
#include <mosaic/platform/AGDK/jni_loader.cpp>
#include <mosaic/platform/AGDK/agdk_platform.hpp>

extern "C"
{
    /*!
     * Handles commands sent to this Android application
     * @param pApp the app the commands are coming from
     * @param cmd the command to handle
     */
    void handle_cmd(android_app* _pApp, int32_t _cmd)
    {
        auto platform = mosaic::core::Platform::getInstance();
        auto context = static_cast<mosaic::platform::agdk::AGDKPlatformContext*>(
            platform->getPlatformContext());

        static bool platformInitialized = false;

        switch (_cmd)
        {
            case APP_CMD_START:
            {
                MOSAIC_DEBUG("Android: APP_CMD_START");

                context->setApp(_pApp);
            }
            break;

            case APP_CMD_RESUME:
            {
                MOSAIC_DEBUG("Android: APP_CMD_RESUME");
            }
            break;

            case APP_CMD_PAUSE:
            {
                MOSAIC_DEBUG("Android: APP_CMD_PAUSE");

                platform->pause();
            }
            break;

            case APP_CMD_STOP:
            {
                MOSAIC_DEBUG("Android: APP_CMD_STOP");

                platform->pause();
            }
            break;

            case APP_CMD_DESTROY:
            {
                MOSAIC_DEBUG("Android: APP_CMD_DESTROY");

                platform->shutdown();
            }
            break;

            case APP_CMD_INIT_WINDOW:
            {
                MOSAIC_DEBUG("Android: APP_CMD_INIT_WINDOW");

                if (_pApp->window != nullptr)
                {
                    context->updateWindow(_pApp->window);

                    if (context->isWindowChanged())
                    {
                        // Notify systems that the window has changed

                        context->acknowledgeWindowChange();
                    }
                }

                if (!platformInitialized)
                {
                    auto result = platform->initialize();

                    if (result.isErr())
                    {
                        MOSAIC_ERROR("Platform initialization failed: {}", result.error());
                        _pApp->destroyRequested = true;
                        return;
                    }

                    platformInitialized = true;
                }

                platform->resume();
            }
            break;

            case APP_CMD_TERM_WINDOW:
            {
                MOSAIC_DEBUG("Android: APP_CMD_TERM_WINDOW");

                context->updateWindow(nullptr);

                if (context->isSurfaceDestroyed())
                {
                    // Notify systems that the surface has been destroyed

                    context->acknowledgeWindowChange();
                }
            }
            break;

            case APP_CMD_WINDOW_RESIZED:
            case APP_CMD_CONFIG_CHANGED:
            {
                if (_pApp->window != nullptr)
                {
                    context->updateWindow(_pApp->window);

                    if (context->isWindowChanged())
                    {
                        // Notify systems that the window has resized or configuration changed

                        context->acknowledgeWindowChange();
                    }
                }
            }
            break;

            default:
                break;
        }
    }

    /*!
     * Enable the motion events you want to handle; not handled events are
     * passed back to OS for further processing. For this example case,
     * only pointer and joystick devices are enabled.
     *
     * @param motionEvent the newly arrived GameActivityMotionEvent.
     * @return true if the event is from a pointer or joystick device,
     *         false for all other input devices.
     */
    bool motion_event_filter_func(const GameActivityMotionEvent* _motionEvent)
    {
        auto sourceClass = _motionEvent->source & AINPUT_SOURCE_CLASS_MASK;
        return (sourceClass == AINPUT_SOURCE_CLASS_POINTER ||
                sourceClass == AINPUT_SOURCE_CLASS_JOYSTICK);
    }
}

#define MOSAIC_ENTRY_POINT(AppType, ...)                                                       \
    extern "C"                                                                                 \
    {                                                                                          \
        void android_main(struct android_app* _pApp)                                           \
        {                                                                                      \
            mosaic::tools::Logger::getInstance()->addSink<mosaic::core::DefaultSink>(          \
                "default", mosaic::core::DefaultSink());                                       \
                                                                                               \
            auto app = std::make_unique<AppType>(__VA_ARGS__);                                 \
            auto platform = mosaic::core::Platform::create(app.get());                         \
                                                                                               \
            _pApp->onAppCmd = handle_cmd;                                                      \
            android_app_set_motion_event_filter(_pApp, motion_event_filter_func);              \
                                                                                               \
            /* Main event loop */                                                              \
            while (!_pApp->destroyRequested)                                                   \
            {                                                                                  \
                /* Process all pending events */                                               \
                int timeout = 0; /* Non-blocking */                                            \
                int events = 0;                                                                \
                android_poll_source* pSource = nullptr;                                        \
                                                                                               \
                /* Process events until no more are available */                               \
                while (true)                                                                   \
                {                                                                              \
                    int pollResult = ALooper_pollOnce(timeout, nullptr, &events,               \
                                                      reinterpret_cast<void**>(&pSource));     \
                                                                                               \
                    if (pollResult == ALOOPER_POLL_TIMEOUT || pollResult == ALOOPER_POLL_WAKE) \
                    {                                                                          \
                        break; /* No more events */                                            \
                    }                                                                          \
                    else if (pollResult == ALOOPER_EVENT_ERROR)                                \
                    {                                                                          \
                        MOSAIC_ERROR("ALooper_pollOnce returned an error");                    \
                        break;                                                                 \
                    }                                                                          \
                    else if (pSource != nullptr)                                               \
                    {                                                                          \
                        pSource->process(_pApp, pSource);                                      \
                    }                                                                          \
                }                                                                              \
                                                                                               \
                /* Run application logic if platform is ready */                               \
                if (!_pApp->destroyRequested)                                                  \
                {                                                                              \
                    auto runResult = platform->run();                                          \
                                                                                               \
                    if (runResult.isErr())                                                     \
                    {                                                                          \
                        MOSAIC_ERROR("Platform run failed: {}", runResult.error());            \
                        break;                                                                 \
                    }                                                                          \
                }                                                                              \
            }                                                                                  \
                                                                                               \
            MOSAIC_DEBUG("Android main loop exiting");                                         \
        }                                                                                      \
    }

#endif
