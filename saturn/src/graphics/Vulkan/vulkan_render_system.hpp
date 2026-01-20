#pragma once

#include "saturn/graphics/render_system.hpp"

#include "context/vulkan_instance.hpp"
#include "context/vulkan_device.hpp"

namespace saturn
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
    pieces::RefResult<core::System, std::string> initialize() override;
    void shutdown() override;

    inline Instance* getInstance() { return &m_instance; }

    inline Device* getDevice() { return &m_device; }
};

} // namespace vulkan
} // namespace graphics
} // namespace saturn
