#include "webgpu_render_system.hpp"

namespace mosaic
{
namespace graphics
{
namespace webgpu
{

pieces::RefResult<RenderSystem, std::string> WebGPURenderSystem::initialize(
    const window::Window* _window)
{
    return pieces::OkRef<RenderSystem, std::string>(*this);
}

void WebGPURenderSystem::shutdown() { destroyAllContexts(); }

} // namespace webgpu
} // namespace graphics
} // namespace mosaic
