#include "mosaic/core/application.hpp"

#include <thread>
#include <chrono>

using namespace std::chrono_literals;

namespace mosaic
{
namespace core
{

bool Application::s_created = false;

Application::Application(const std::string& _appName)
    : m_exitRequested(false),
      m_appName(_appName),
      m_state(ApplicationState::uninitialized),
      m_windowSystem(mosaic::window::WindowSystem::create()),
      m_inputSystem(std::make_unique<input::InputSystem>()),
      m_renderSystem(graphics::RenderSystem::create(graphics::RendererAPIType::vulkan))

{
    assert(!s_created && "Application instance already exists!");

    s_created = true;
}

pieces::RefResult<Application, std::string> Application::initialize()
{
    if (m_state != ApplicationState::uninitialized)
    {
        return pieces::ErrRef<Application, std::string>("Application already initialized");
    }

    m_windowSystem->initialize();

    auto wndCreateResult = m_windowSystem->createWindow("MainWindow");

    if (wndCreateResult.isErr())
    {
        return pieces::ErrRef<Application, std::string>(std::move(wndCreateResult.error()));
    }

    m_inputSystem->initialize();

    auto inputCtxRegResult = m_inputSystem->registerWindow(wndCreateResult.unwrap());

    if (inputCtxRegResult.isErr())
    {
        return pieces::ErrRef<Application, std::string>(std::move(inputCtxRegResult.error()));
    }

    m_renderSystem->initialize();

    auto rndrCtxCreateRes = m_renderSystem->createContext(wndCreateResult.unwrap());

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
        MOSAIC_ERROR("Application initialization error: {}", e.what());
        return pieces::ErrRef<Application, std::string>(
            std::string("Application initialization error: ") + e.what());
    }

    m_state = ApplicationState::initialized;

    return pieces::OkRef<Application, std::string>(*this);
}

pieces::RefResult<Application, std::string> Application::update()
{
    if (m_state != ApplicationState::resumed)
    {
        return pieces::OkRef<Application, std::string>(*this);
    }

    core::Timer::tick();

    m_windowSystem->update();

    m_inputSystem->update();

    try
    {
        onPollInputs();
    }
    catch (const std::exception& e)
    {
        MOSAIC_ERROR("Application shutdown error: {}", e.what());
        return pieces::ErrRef<Application, std::string>(
            std::string("Application input polling error: ") + e.what());
    }

    m_renderSystem->update();

    try
    {
        onUpdate();
    }
    catch (const std::exception& e)
    {
        MOSAIC_ERROR("Application shutdown error: {}", e.what());
        return pieces::ErrRef<Application, std::string>(std::string("Application update error: ") +
                                                        e.what());
    }

    return pieces::OkRef<Application, std::string>(*this);
}

void Application::pause()
{
    if (m_state == ApplicationState::resumed)
    {
        try
        {
            onPause();
        }
        catch (const std::exception& e)
        {
            MOSAIC_ERROR("Application pause error: {}", e.what());
        }

        m_state = ApplicationState::paused;
    }
}

void Application::resume()
{
    if (m_state == ApplicationState::initialized || m_state == ApplicationState::paused)
    {
        try
        {
            onResume();
        }
        catch (const std::exception& e)
        {
            MOSAIC_ERROR("Application resume error: {}", e.what());
        }

        m_state = ApplicationState::resumed;
    }
}

void Application::shutdown()
{
    if (m_state != ApplicationState::shutdown)
    {
        try
        {
            onShutdown();
        }
        catch (const std::exception& e)
        {
            MOSAIC_ERROR("Application shutdown error: {}", e.what());
        }

        m_renderSystem->shutdown();
        m_inputSystem->shutdown();
        m_windowSystem->shutdown();

        m_state = ApplicationState::shutdown;
    }
}

} // namespace core
} // namespace mosaic
