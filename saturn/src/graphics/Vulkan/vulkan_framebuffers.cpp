#include "vulkan_framebuffers.hpp"

namespace saturn
{
namespace graphics
{
namespace vulkan
{

void createSwapchainFramebuffers(Swapchain& _swapchain, const Device& _device,
                                 const RenderPass& _renderPass)
{
    _swapchain.framebuffers.resize(_swapchain.imageViews.size());

    for (size_t i = 0; i < _swapchain.imageViews.size(); i++)
    {
        VkImageView attachments[] = {
            _swapchain.imageViews[i],
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.pNext = nullptr;
        framebufferInfo.flags = 0;
        framebufferInfo.renderPass = _renderPass.renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = _swapchain.extent.width;
        framebufferInfo.height = _swapchain.extent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(_device.device, &framebufferInfo, nullptr,
                                &_swapchain.framebuffers[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

void destroySwapchainFramebuffers(Swapchain& _swapchain, const Device& _device)
{
    for (auto framebuffer : _swapchain.framebuffers)
    {
        vkDestroyFramebuffer(_device.device, framebuffer, nullptr);
    }
}

} // namespace vulkan
} // namespace graphics
} // namespace saturn
