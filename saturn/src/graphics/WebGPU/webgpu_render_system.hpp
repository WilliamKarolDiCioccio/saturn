#pragma once

#include "saturn/graphics/render_system.hpp"

#include "webgpu_instance.hpp"
#include "webgpu_device.hpp"

namespace saturn
{
namespace graphics
{
namespace webgpu
{

class WebGPURenderSystem : public RenderSystem
{
   public:
    WebGPURenderSystem() : RenderSystem(RendererAPIType::web_gpu) {};
    ~WebGPURenderSystem() override = default;

   public:
    pieces::RefResult<core::System, std::string> initialize() override;
    void shutdown() override;
};

} // namespace webgpu
} // namespace graphics
} // namespace saturn
