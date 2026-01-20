#pragma once

#include <memory>

#include <saturn/core/sys_ui.hpp>
#include <saturn/core/sys_console.hpp>
#include <saturn/tools/logger.hpp>
#include <saturn/tools/logger_default_sink.hpp>
#include <saturn/tools/tracer.hpp>
#include <saturn/core/cmd_line_parser.hpp>
#include <saturn/core/platform.hpp>
#include <saturn/core/application.hpp>

#ifndef SATURN_PLATFORM_ANDROID

namespace saturn
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

    logger->addSink<tools::DefaultSink>("default", tools::DefaultSink());

    tools::Tracer::initialize();

    // This scope guard ensures all resources have been disposed before shutting down the logger and
    // tracer.

    {
        auto app = std::make_unique<AppType>(std::forward<Args>(_appConstuctorArgs)...);

        auto platform = core::Platform::create(app.get());

        auto result = platform->initialize().andThen(std::mem_fn(&core::Platform::run));

        if (result.isErr())
        {
            SATURN_ERROR(result.error().c_str());
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

} // namespace saturn

#endif

#if defined(SATURN_PLATFORM_WINDOWS)

#include <windows.h>

#include <saturn/platform/Win32/wstring.hpp>

#define SATURN_ENTRY_POINT(AppType, ...)                                            \
    int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,  \
                       _In_ PSTR lpCmdLine, _In_ int nCmdShow)                      \
    {                                                                               \
        int argc = 0;                                                               \
        LPWSTR* argv_w = CommandLineToArgvW(GetCommandLineW(), &argc);              \
                                                                                    \
        if (argv_w == nullptr)                                                      \
        {                                                                           \
            saturn::core::SystemUI::showErrorDialog("Error parsing cmd line args!", \
                                                    "argv_w is nullptr.");          \
            return 1;                                                               \
        }                                                                           \
                                                                                    \
        std::vector<std::string> args;                                              \
        args.reserve(argc);                                                         \
        for (int i = 0; i < argc; ++i)                                              \
        {                                                                           \
            args.emplace_back(saturn::platform::win32::WStringToString(argv_w[i])); \
        }                                                                           \
                                                                                    \
        LocalFree(argv_w);                                                          \
                                                                                    \
        return saturn::runApp<AppType>(args __VA_OPT__(, __VA_ARGS__));             \
    }

#elif defined(SATURN_PLATFORM_LINUX) || defined(SATURN_PLATFORM_MACOS) || \
    defined(SATURN_PLATFORM_EMSCRIPTEN)

#define SATURN_ENTRY_POINT(AppType, ...)                                \
    int main(int _argc, char** _argv)                                   \
    {                                                                   \
        std::vector<std::string> args;                                  \
        args.reserve(_argc);                                            \
        for (int i = 0; i < _argc; ++i)                                 \
        {                                                               \
            args.emplace_back(_argv[i]);                                \
        }                                                               \
                                                                        \
        return saturn::runApp<AppType>(args __VA_OPT__(, __VA_ARGS__)); \
    }

#elif defined(SATURN_PLATFORM_ANDROID)

#include <saturn/platform/AGDK/jni_helper.hpp>
#include <saturn/platform/AGDK/jni_loader.cpp>
#include <saturn/platform/AGDK/agdk_platform.hpp>

extern "C"
{
    /*!
     * Handles commands sent to this Android application
     * @param pApp the app the commands are coming from
     * @param cmd the command to handle
     */
    void handle_cmd(android_app* _pApp, int32_t _cmd)
    {
        auto platform = saturn::core::Platform::getInstance();
        auto context = static_cast<saturn::platform::agdk::AGDKPlatformContext*>(
            platform->getPlatformContext());

        static bool platformInitialized = false;

        switch (_cmd)
        {
            case APP_CMD_START:
            {
                SATURN_DEBUG("Android: APP_CMD_START");

                context->setApp(_pApp);
            }
            break;

            case APP_CMD_RESUME:
            {
                SATURN_DEBUG("Android: APP_CMD_RESUME");
            }
            break;

            case APP_CMD_PAUSE:
            {
                SATURN_DEBUG("Android: APP_CMD_PAUSE");

                platform->pause();
            }
            break;

            case APP_CMD_STOP:
            {
                SATURN_DEBUG("Android: APP_CMD_STOP");

                platform->pause();
            }
            break;

            case APP_CMD_DESTROY:
            {
                SATURN_DEBUG("Android: APP_CMD_DESTROY");

                platform->shutdown();
            }
            break;

            case APP_CMD_INIT_WINDOW:
            {
                SATURN_DEBUG("Android: APP_CMD_INIT_WINDOW");

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
                        SATURN_ERROR("Platform initialization failed: {}", result.error());
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
                SATURN_DEBUG("Android: APP_CMD_TERM_WINDOW");

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

#define SATURN_ENTRY_POINT(AppType, ...)                                                       \
    extern "C"                                                                                 \
    {                                                                                          \
        void android_main(struct android_app* _pApp)                                           \
        {                                                                                      \
            saturn::tools::Logger::getInstance()->addSink<saturn::tools::DefaultSink>(         \
                "default", saturn::tools::DefaultSink());                                      \
                                                                                               \
            auto app = std::make_unique<AppType>(__VA_ARGS__);                                 \
            auto platform = saturn::core::Platform::create(app.get());                         \
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
                        SATURN_ERROR("ALooper_pollOnce returned an error");                    \
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
                        SATURN_ERROR("Platform run failed: {}", runResult.error());            \
                        break;                                                                 \
                    }                                                                          \
                }                                                                              \
            }                                                                                  \
                                                                                               \
            SATURN_DEBUG("Android main loop exiting");                                         \
        }                                                                                      \
    }

#endif
