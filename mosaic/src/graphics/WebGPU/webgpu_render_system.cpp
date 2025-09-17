#include "webgpu_render_system.hpp"

namespace mosaic
{
namespace graphics
{
namespace webgpu
{

pieces::RefResult<core::System, std::string> WebGPURenderSystem::initialize()
{
    return pieces::OkRef<core::System, std::string>(*this);
}

void WebGPURenderSystem::shutdown() { destroyAllContexts(); }

} // namespace webgpu
} // namespace graphics
} // namespace mosaic
