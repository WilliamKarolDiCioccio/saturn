#include "vulkan_render_context.hpp"

#if defined(SATURN_PLATFORM_ANDROID)
#include "saturn/platform/AGDK/agdk_platform.hpp"
#endif

#include "vulkan_render_system.hpp"

namespace saturn
{
namespace graphics
{
namespace vulkan
{

VulkanRenderContext::VulkanRenderContext(const window::Window* _window,
                                         const RenderContextSettings& _settings)
    : RenderContext(_window, _settings),
      m_instance(nullptr),
      m_device(nullptr),
      m_currentFrame(0),
      m_framebufferResized(false){};

pieces::RefResult<RenderContext, std::string> VulkanRenderContext::initialize(
    RenderSystem* _renderSystem)
{
    m_instance = static_cast<VulkanRenderSystem*>(_renderSystem)->getInstance();
    m_device = static_cast<VulkanRenderSystem*>(_renderSystem)->getDevice();

    auto window = getWindowInternal();
    auto& settings = getSettings();

    m_frameData.resize(getSettings().backbufferCount);

    createSurface(m_surface, *m_instance, window->getNativeHandle());

    createSwapchain(m_swapchain, *m_device, m_surface, window->getNativeHandle(),
                    window->getFramebufferSize(), window->getWindowProperties().isFullscreen);

    createRenderPass(m_renderPass, *m_device, m_swapchain);
    createGraphicsPipeline(m_pipeline, *m_device, m_swapchain, m_renderPass);
    createSwapchainFramebuffers(m_swapchain, *m_device, m_renderPass);
    createCommandPool(m_commandPool, *m_device, m_surface);

    createFrames();

#if defined(SATURN_PLATFORM_DESKTOP) || defined(SATURN_PLATFORM_WEB)

    window->registerWindowResizeCallback([this](int height, int width)
                                         { m_framebufferResized = true; });

#elif defined(SATURN_PLATFORM_ANDROID)

    auto platform = static_cast<platform::agdk::AGDKPlatform*>(core::Platform::getInstance());

    platform->getPlatformContext()->registerPlatformContextChangedCallback(
        [this](void* _context)
        {
            auto context = static_cast<platform::agdk::AGDKPlatformContext*>(_context);

            if (context->isWindowChanged()) recreateSurface();
        });

#endif

    return pieces::OkRef<RenderContext, std::string>(*this);
}

void VulkanRenderContext::shutdown()
{
    vkDeviceWaitIdle(m_device->device);

    destroyFrames();

    destroyCommandPool(m_commandPool, *m_device);
    destroySwapchainFramebuffers(m_swapchain, *m_device);
    destroyGraphicsPipeline(m_pipeline, *m_device);
    destroyRenderPass(m_renderPass, *m_device);
    destroySwapchain(m_swapchain);
    destroySurface(m_surface, *m_instance);
}

void VulkanRenderContext::resizeFramebuffer()
{
    auto window = getWindowInternal();

    auto framebufferSize = window->getFramebufferSize();

    while (framebufferSize.x * framebufferSize.y == 0)
    {
        framebufferSize = window->getFramebufferSize();
    }

    vkDeviceWaitIdle(m_device->device);

    destroySwapchainFramebuffers(m_swapchain, *m_device);
    destroyGraphicsPipeline(m_pipeline, *m_device);
    destroyRenderPass(m_renderPass, *m_device);
    destroySwapchain(m_swapchain);

    createSwapchain(m_swapchain, *m_device, m_surface, window->getNativeHandle(), framebufferSize,
                    window->getWindowProperties().isFullscreen);

    createRenderPass(m_renderPass, *m_device, m_swapchain);
    createGraphicsPipeline(m_pipeline, *m_device, m_swapchain, m_renderPass);
    createSwapchainFramebuffers(m_swapchain, *m_device, m_renderPass);

    m_framebufferResized = false;
}

void VulkanRenderContext::recreateSurface()
{
    auto window = getWindowInternal();
    auto windowProps = window->getWindowProperties();

    if (!window->getNativeHandle()) return;

    vkDeviceWaitIdle(m_device->device);

    destroySwapchainFramebuffers(m_swapchain, *m_device);
    destroyGraphicsPipeline(m_pipeline, *m_device);
    destroyRenderPass(m_renderPass, *m_device);
    destroySwapchain(m_swapchain);
    destroySurface(m_surface, *m_instance);

    createSurface(m_surface, *m_instance, window->getNativeHandle());
    createSwapchain(m_swapchain, *m_device, m_surface, window->getNativeHandle(),
                    window->getFramebufferSize(), window->getWindowProperties().isFullscreen);
    createRenderPass(m_renderPass, *m_device, m_swapchain);
    createGraphicsPipeline(m_pipeline, *m_device, m_swapchain, m_renderPass);
    createSwapchainFramebuffers(m_swapchain, *m_device, m_renderPass);
}

void VulkanRenderContext::beginFrame()
{
    auto& frame = m_frameData[m_currentFrame];

    // Wait for previous frame to finish
    vkWaitForFences(m_device->device, 1, &frame.inFlightFence, VK_TRUE, UINT64_MAX);

    // Acquire next image
    VkResult acquireResult =
        vkAcquireNextImageKHR(m_device->device, m_swapchain.swapchain, UINT64_MAX,
                              frame.imageAvailableSemaphore, VK_NULL_HANDLE, &frame.imageIndex);

    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR)
    {
        resizeFramebuffer();
        return;
    }
    else if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR)
    {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    // Prepare for new frame
    vkResetFences(m_device->device, 1, &frame.inFlightFence);
    vkResetCommandBuffer(frame.commandBuffer, 0);
}

void VulkanRenderContext::updateResources()
{
    // For future per-frame resource updates (uniforms, descriptor sets, etc.)
    // Currently empty
}

void VulkanRenderContext::drawScene()
{
    auto& frame = m_frameData[m_currentFrame];

    // Begin recording commands
    beingCommandBuffer(frame.commandBuffer, m_surface);

    bindGraphicsPipeline(m_pipeline, frame.commandBuffer);

    // Setup viewport
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_swapchain.extent.width);
    viewport.height = static_cast<float>(m_swapchain.extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(frame.commandBuffer, 0, 1, &viewport);

    // Setup scissor
    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = m_swapchain.extent;
    vkCmdSetScissor(frame.commandBuffer, 0, 1, &scissor);

    // Render pass and drawing
    beginRenderPass(m_renderPass, m_swapchain, frame.commandBuffer, frame.imageIndex);
    vkCmdDraw(frame.commandBuffer, 3, 1, 0, 0);
    endRenderPass(frame.commandBuffer);

    // End recording
    endCommandBuffer(frame.commandBuffer);
}

void VulkanRenderContext::endFrame()
{
    auto& frame = m_frameData[m_currentFrame];

    // Submit draw command
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {frame.imageAvailableSemaphore};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &frame.commandBuffer;

    VkSemaphore signalSemaphores[] = {frame.renderFinishedSemaphore};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(m_device->graphicsQueue, 1, &submitInfo, frame.inFlightFence) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    // Present the rendered image
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {m_swapchain.swapchain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &frame.imageIndex;

    VkResult presentResult = vkQueuePresentKHR(m_device->presentQueue, &presentInfo);
    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR ||
        m_framebufferResized)
    {
        resizeFramebuffer();
    }
    else if (presentResult != VK_SUCCESS)
    {
        throw std::runtime_error("failed to present swap chain image!");
    }

    // Advance frame
    m_currentFrame = (m_currentFrame + 1) % getSettings().backbufferCount;
}

void VulkanRenderContext::createFrames()
{
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (auto& frame : m_frameData)
    {
        createCommandBuffer(frame.commandBuffer, *m_device, m_commandPool);

        if (vkCreateSemaphore(m_device->device, &semaphoreInfo, nullptr,
                              &frame.imageAvailableSemaphore) != VK_SUCCESS ||
            vkCreateSemaphore(m_device->device, &semaphoreInfo, nullptr,
                              &frame.renderFinishedSemaphore) != VK_SUCCESS ||
            vkCreateFence(m_device->device, &fenceInfo, nullptr, &frame.inFlightFence) !=
                VK_SUCCESS)
        {
            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }
}

void VulkanRenderContext::destroyFrames()
{
    for (auto& frame : m_frameData)
    {
        vkDestroySemaphore(m_device->device, frame.imageAvailableSemaphore, nullptr);
        vkDestroySemaphore(m_device->device, frame.renderFinishedSemaphore, nullptr);
        vkDestroyFence(m_device->device, frame.inFlightFence, nullptr);

        destroyCommandBuffer(frame.commandBuffer, *m_device, m_commandPool);
    }
}

} // namespace vulkan
} // namespace graphics
} // namespace saturn
