#include "saturn/core/application.hpp"

#include <cassert>
#include <exception>
#include <memory>
#include <string>
#include <utility>
#include <saturn/tools/logger.hpp>
#include <saturn/core/timer.hpp>
#include <saturn/graphics/render_system.hpp>
#include <saturn/input/input_system.hpp>
#include <saturn/window/window_system.hpp>
#include <pieces/core/result.hpp>

using namespace std::chrono_literals;

namespace saturn
{
namespace core
{

struct Application::Impl
{
    static bool s_created;
    bool exitRequested = false;
    std::string appName;
    ApplicationState state = ApplicationState::uninitialized;

    std::unique_ptr<window::WindowSystem> windowSystem;
    std::unique_ptr<input::InputSystem> inputSystem;
    std::unique_ptr<graphics::RenderSystem> renderSystem;

    Impl(const std::string& _name)
        : exitRequested(false),
          appName(_name),
          state(ApplicationState::uninitialized),
          windowSystem(saturn::window::WindowSystem::create()),
          inputSystem(std::make_unique<input::InputSystem>()),
          renderSystem(graphics::RenderSystem::create(graphics::RendererAPIType::vulkan))
    {
        assert(!s_created && "Application  already exists!");
        s_created = true;
    };

    ~Impl() { s_created = false; }
};

bool Application::Impl::s_created = false;

Application::Application(const std::string& _appName) : m_impl(new Impl(_appName)) {}

Application::~Application() { delete m_impl; }

pieces::RefResult<Application, std::string> Application::initialize()
{
    if (m_impl->state != ApplicationState::uninitialized)
    {
        return pieces::ErrRef<Application, std::string>("Application already initialized");
    }

    m_impl->windowSystem->initialize();

    auto wndCreateResult = m_impl->windowSystem->createWindow("MainWindow");

    if (wndCreateResult.isErr())
    {
        return pieces::ErrRef<Application, std::string>(std::move(wndCreateResult.error()));
    }

    m_impl->inputSystem->initialize();

    auto inputCtxRegResult = m_impl->inputSystem->registerWindow(wndCreateResult.unwrap());

    if (inputCtxRegResult.isErr())
    {
        return pieces::ErrRef<Application, std::string>(std::move(inputCtxRegResult.error()));
    }

    m_impl->renderSystem->initialize();

    auto rndrCtxCreateRes = m_impl->renderSystem->createContext(wndCreateResult.unwrap());

    if (rndrCtxCreateRes.isErr())
    {
        return pieces::ErrRef<Application, std::string>(std::move(inputCtxRegResult.error()));
    }

    try
    {
        onInitialize();
    }
    catch (const std::exception& e)
    {
        SATURN_ERROR("Application initialization error: {}", e.what());
        return pieces::ErrRef<Application, std::string>(
            std::string("Application initialization error: ") + e.what());
    }

    m_impl->state = ApplicationState::initialized;

    return pieces::OkRef<Application, std::string>(*this);
}

pieces::RefResult<Application, std::string> Application::update()
{
    if (m_impl->state != ApplicationState::resumed)
    {
        return pieces::OkRef<Application, std::string>(*this);
    }

    core::Timer::tick();

    m_impl->windowSystem->update();

    m_impl->inputSystem->update();

    try
    {
        onPollInputs();
    }
    catch (const std::exception& e)
    {
        SATURN_ERROR("Application shutdown error: {}", e.what());
        return pieces::ErrRef<Application, std::string>(
            std::string("Application input polling error: ") + e.what());
    }

    m_impl->renderSystem->update();

    try
    {
        onUpdate();
    }
    catch (const std::exception& e)
    {
        SATURN_ERROR("Application shutdown error: {}", e.what());
        return pieces::ErrRef<Application, std::string>(std::string("Application update error: ") +
                                                        e.what());
    }

    return pieces::OkRef<Application, std::string>(*this);
}

void Application::pause()
{
    if (m_impl->state == ApplicationState::resumed)
    {
        try
        {
            onPause();
        }
        catch (const std::exception& e)
        {
            SATURN_ERROR("Application pause error: {}", e.what());
        }

        m_impl->state = ApplicationState::paused;
    }
}

void Application::resume()
{
    if (m_impl->state == ApplicationState::initialized || m_impl->state == ApplicationState::paused)
    {
        try
        {
            onResume();
        }
        catch (const std::exception& e)
        {
            SATURN_ERROR("Application resume error: {}", e.what());
        }

        m_impl->state = ApplicationState::resumed;
    }
}

void Application::shutdown()
{
    if (m_impl->state != ApplicationState::shutdown)
    {
        try
        {
            onShutdown();
        }
        catch (const std::exception& e)
        {
            SATURN_ERROR("Application shutdown error: {}", e.what());
        }

        m_impl->renderSystem->shutdown();
        m_impl->inputSystem->shutdown();
        m_impl->windowSystem->shutdown();

        m_impl->state = ApplicationState::shutdown;
    }
}

bool Application::shouldExit() const { return m_impl->exitRequested; }

void Application::requestExit() { m_impl->exitRequested = true; }

ApplicationState Application::getState() const { return m_impl->state; }

bool Application::isInitialized() const { return m_impl->state != ApplicationState::uninitialized; }

bool Application::isPaused() const { return m_impl->state == ApplicationState::paused; }

bool Application::isResumed() const { return m_impl->state == ApplicationState::resumed; }

window::WindowSystem* Application::getWindowSystem() const { return m_impl->windowSystem.get(); }

input::InputSystem* Application::getInputSystem() const { return m_impl->inputSystem.get(); }

graphics::RenderSystem* Application::getRenderSystem() const { return m_impl->renderSystem.get(); }

} // namespace core
} // namespace saturn
