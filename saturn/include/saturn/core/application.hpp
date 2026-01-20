#pragma once

#include <string>

#include <pieces/core/result.hpp>

#include "saturn/defines.hpp"
#include "saturn/version.hpp"

namespace saturn
{

namespace window
{
class WindowSystem;
}

namespace input
{
class InputSystem;
}

namespace graphics
{
class RenderSystem;
}

namespace core
{

enum class ApplicationState
{
    uninitialized,
    initialized,
    resumed,
    paused,
    shutdown
};

class Application;

template <typename AppType>
concept IsApplication =
    std::derived_from<AppType, core::Application> && !std::is_abstract_v<AppType>;

/**
 * @brief Base class for applications in the Saturn framework.
 *
 * This class provides the main application lifecycle management, including
 * initialization, update, pause, resume, and shutdown methods. It also manages
 * the application state and provides access to the engine's systems such as
 * window, input, and rendering.
 *
 * @note The application must override the `on...` methods to provide specific
 * functionality for initialization, update, pause, resume, and shutdown.
 *
 * @note The application must not be instantiated directly. Instead, it created automatically
 * in the SATURN_ENTRY_POINT macro, which is the entry point for the application.
 */
class SATURN_API Application
{
   private:
    struct Impl;

    Impl* m_impl;

   public:
    Application(const std::string& _appName);
    virtual ~Application();

    Application(Application&) = delete;
    Application& operator=(Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(Application&&) = delete;

   public:
    pieces::RefResult<Application, std::string> initialize();
    pieces::RefResult<Application, std::string> update();
    void pause();
    void resume();
    void shutdown();

    [[nodiscard]] bool shouldExit() const;
    void requestExit();

    [[nodiscard]] ApplicationState getState() const;
    [[nodiscard]] bool isInitialized() const;
    [[nodiscard]] bool isPaused() const;
    [[nodiscard]] bool isResumed() const;

    [[nodiscard]] window::WindowSystem* getWindowSystem() const;
    [[nodiscard]] input::InputSystem* getInputSystem() const;
    [[nodiscard]] graphics::RenderSystem* getRenderSystem() const;

   protected:
    virtual void onInitialize() = 0;
    virtual void onUpdate() = 0;
    virtual void onPause() = 0;
    virtual void onResume() = 0;
    virtual void onShutdown() = 0;
    virtual void onPollInputs() = 0;
};

} // namespace core
} // namespace saturn
