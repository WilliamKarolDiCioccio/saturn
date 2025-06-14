#pragma once

#include "mosaic/graphics/render_system.hpp"

#include "context/vulkan_instance.hpp"
#include "context/vulkan_device.hpp"

namespace mosaic
{
namespace graphics
{
namespace vulkan
{

class VulkanRenderSystem : public RenderSystem
{
   private:
    Instance m_instance;
    Device m_device;

   public:
    VulkanRenderSystem() : RenderSystem(RendererAPIType::vulkan) {};
    ~VulkanRenderSystem() override = default;

   public:
    pieces::RefResult<RenderSystem, std::string> initialize(const window::Window* _window) override;
    void shutdown() override;

    inline Instance* getInstance() { return &m_instance; }

    inline Device* getDevice() { return &m_device; }
};

} // namespace vulkan
} // namespace graphics
} // namespace mosaic
