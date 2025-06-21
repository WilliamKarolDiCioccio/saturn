#pragma once

#include <memory>

#include <mosaic/core/logger.hpp>
#include <mosaic/core/tracer.hpp>
#include <mosaic/core/application.hpp>
#include <mosaic/core/platform.hpp>

#ifndef MOSAIC_PLATFORM_ANDROID

namespace mosaic
{

template <typename AppType, typename... Args>
    requires core::IsApplication<AppType>
int runApp(Args&&... args)
{
    core::LoggerManager::initialize();

    core::LoggerManager::getInstance()->addSink<core::DefaultSink>("default", core::DefaultSink());

    // This scope guard ensures all resources have been disposed before shutting down the logger and
    // tracer.

    {
        auto app = std::make_unique<AppType>(std::forward<Args>(args)...);

        auto platform = core::Platform::create(app.get());

        auto result = platform->initialize().andThen(std::mem_fn(&core::Platform::run));

        if (result.isErr())
        {
            MOSAIC_ERROR(result.error().c_str());
            return 1;
        }

        platform->shutdown();
    }

    core::LoggerManager::shutdown();

    return 0;
}

} // namespace mosaic

#endif

#if defined(MOSAIC_PLATFORM_WINDOWS)

#include <windows.h>

#if defined(MOSAIC_DEBUG_BUILD) || defined(MOSAIC_DEV_BUILD)

#define MOSAIC_ENTRY_POINT(AppType, ...)                                           \
    int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, \
                       _In_ PSTR lpCmdLine, _In_ int nCmdShow)                     \
    {                                                                              \
        AllocConsole();                                                            \
                                                                                   \
        FILE* fp;                                                                  \
        freopen_s(&fp, "CONOUT$", "w", stdout);                                    \
        freopen_s(&fp, "CONOUT$", "w", stderr);                                    \
        freopen_s(&fp, "CONIN$", "r", stdin);                                      \
        std::ios::sync_with_stdio(true);                                           \
                                                                                   \
        return mosaic::runApp<AppType>(__VA_ARGS__);                               \
                                                                                   \
        FreeConsole();                                                             \
    }

#else

#define MOSAIC_ENTRY_POINT(AppType, ...)                                           \
    int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, \
                       _In_ PSTR lpCmdLine, _In_ int nCmdShow)                     \
    {                                                                              \
        return mosaic::runApp<AppType>(__VA_ARGS__);                               \
    }

#endif

#elif defined(MOSAIC_PLATFORM_LINUX) || defined(MOSAIC_PLATFORM_MACOS) || \
    defined(MOSAIC_PLATFORM_EMSCRIPTEN)

#define MOSAIC_ENTRY_POINT(AppType, ...) \
    int main(int argc, char** argv) { return mosaic::runApp<AppType>(__VA_ARGS__); }

#elif defined(MOSAIC_PLATFORM_ANDROID)

#include <jni.h>
#include <game-activity/GameActivity.cpp>
#include <game-text-input/gametextinput.cpp>

#include <mosaic/platform/AGDK/jni_helper.hpp>
#include <mosaic/platform/AGDK/agdk_platform.hpp>

extern "C"
{

#include <game-activity/native_app_glue/android_native_app_glue.c>

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

JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    mosaic::core::LoggerManager::initialize();

    auto helper = mosaic::platform::agdk::JNIHelper::getInstance();

    helper->initialize(vm);

    auto clazzRef = helper->findClass("com/mosaic/engine_bridge/EngineBridge");

    if (!clazzRef)
    {
        MOSAIC_ERROR("Failed to find EngineBridge class");
        return JNI_ERR;
    }

    helper->createGlobalRef(clazzRef);

    helper->getStaticMethodID("com/mosaic/engine_bridge/EngineBridge", "showInfoDialog",
                              "(Ljava/lang/String;Ljava/lang/String;)V");

    helper->getStaticMethodID("com/mosaic/engine_bridge/EngineBridge", "showWarningDialog",
                              "(Ljava/lang/String;Ljava/lang/String;)V");

    helper->getStaticMethodID("com/mosaic/engine_bridge/EngineBridge", "showErrorDialog",
                              "(Ljava/lang/String;Ljava/lang/String;)V");

    helper->getStaticMethodID("com/mosaic/engine_bridge/EngineBridge", "showQuestionDialog",
                              "(Ljava/lang/String;Ljava/lang/String;Z)Ljava/lang/Boolean;");

    return JNI_VERSION_1_6;
}

JNIEXPORT void JNICALL JNI_OnUnload(JavaVM* vm, void* /*reserved*/)
{
    mosaic::platform::agdk::JNIHelper::getInstance()->shutdown();

    mosaic::core::LoggerManager::shutdown();
}

#define MOSAIC_ENTRY_POINT(AppType, ...)                                                       \
    extern "C"                                                                                 \
    {                                                                                          \
        void android_main(struct android_app* _pApp)                                           \
        {                                                                                      \
            mosaic::core::LoggerManager::getInstance()->addSink<mosaic::core::DefaultSink>(    \
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
