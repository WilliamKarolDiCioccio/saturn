#include "mosaic/graphics/render_context.hpp"

namespace mosaic
{
namespace graphics
{

RenderContext::RenderContext(const window::Window* _window, const RenderContextSettings& _settings)
    : m_window(_window), m_settings(_settings) {};

void RenderContext::render()
{
    beginFrame();
    updateResources();
    drawScene();
    endFrame();
}

} // namespace graphics
} // namespace mosaic
