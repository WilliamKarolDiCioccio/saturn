#include "saturn/graphics/render_context.hpp"

#include "saturn/window/window.hpp"

namespace saturn
{
namespace graphics
{

struct RenderContext::Impl
{
    const window::Window* m_window;
    RenderContextSettings m_settings;

    Impl(const window::Window* _window, const RenderContextSettings& _settings)
        : m_window(_window), m_settings(_settings){};
};

RenderContext::RenderContext(const window::Window* _window, const RenderContextSettings& _settings)
    : m_impl(new Impl(_window, _settings)){};

RenderContext::~RenderContext() { delete m_impl; }

void RenderContext::render()
{
    beginFrame();
    updateResources();
    drawScene();
    endFrame();
}

const window::Window* RenderContext::getWindow() const { return m_impl->m_window; }

const RenderContextSettings RenderContext::getSettings() const { return m_impl->m_settings; }

window::Window* RenderContext::getWindowInternal()
{
    return const_cast<window::Window*>(m_impl->m_window);
}

RenderContextSettings& RenderContext::getSettingsInternal() { return m_impl->m_settings; }

} // namespace graphics
} // namespace saturn
