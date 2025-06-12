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

    m_window = mosaic::core::Window::create("Testbed", glm::vec2(1280, 720));

    m_inputSystem->initialize();
    m_renderSystem->initialize(m_window.get());

    auto result = onInitialize();

    if (result.has_value())
    {
        return pieces::ErrRef<Application, std::string>(std::move(result.value()));
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

    m_inputSystem->poll();
    m_renderSystem->render();

    auto result = onUpdate();

    if (result.has_value())
    {
        return pieces::ErrRef<Application, std::string>(std::move(result.value()));
    }

    return pieces::OkRef<Application, std::string>(*this);
}

void Application::pause()
{
    if (m_state == ApplicationState::resumed)
    {
        onPause();

        m_state = ApplicationState::paused;
    }
}

void Application::resume()
{
    if (m_state == ApplicationState::initialized || m_state == ApplicationState::paused)
    {
        onResume();

        m_state = ApplicationState::resumed;
    }
}

void Application::shutdown()
{
    if (m_state != ApplicationState::shutdown)
    {
        onShutdown();

        m_inputSystem->shutdown();
        m_renderSystem->shutdown();

        m_state = ApplicationState::shutdown;
    }
}

} // namespace core
} // namespace mosaic
