#pragma once

#include "saturn/graphics/render_context.hpp"

#include "context/vulkan_instance.hpp"
#include "context/vulkan_device.hpp"
#include "context/vulkan_surface.hpp"
#include "vulkan_swapchain.hpp"
#include "commands/vulkan_render_pass.hpp"
#include "pipelines/vulkan_pipeline.hpp"
#include "vulkan_framebuffers.hpp"
#include "commands/vulkan_command_pool.hpp"
#include "commands/vulkan_command_buffer.hpp"

namespace saturn
{
namespace graphics
{
namespace vulkan
{

class VulkanRenderContext : public RenderContext
{
   private:
    struct FrameData
    {
        VkSemaphore imageAvailableSemaphore;
        VkSemaphore renderFinishedSemaphore;
        VkFence inFlightFence;
        CommandBuffer commandBuffer;
        uint32_t imageIndex;

        FrameData()
            : imageAvailableSemaphore(nullptr),
              renderFinishedSemaphore(nullptr),
              inFlightFence(nullptr),
              commandBuffer(nullptr),
              imageIndex(0){};
    };

    const Instance* m_instance;
    const Device* m_device;

    Surface m_surface;
    Swapchain m_swapchain;
    RenderPass m_renderPass;
    Pipeline m_pipeline;
    CommandPool m_commandPool;

    uint32_t m_currentFrame;
    std::vector<FrameData> m_frameData;

    bool m_framebufferResized;

   public:
    VulkanRenderContext(const window::Window* _window, const RenderContextSettings& _settings);
    ~VulkanRenderContext() override = default;

   public:
    pieces::RefResult<RenderContext, std::string> initialize(RenderSystem* _renderSystem) override;
    void shutdown() override;

   private:
    void resizeFramebuffer() override;
    void recreateSurface() override;
    void beginFrame() override;
    void updateResources() override;
    void drawScene() override;
    void endFrame() override;

    void createFrames();
    void destroyFrames();
};

} // namespace vulkan
} // namespace graphics
} // namespace saturn
