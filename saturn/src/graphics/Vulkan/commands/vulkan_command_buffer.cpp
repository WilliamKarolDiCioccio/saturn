#include "vulkan_command_buffer.hpp"

namespace saturn
{
namespace graphics
{
namespace vulkan
{

void createCommandBuffer(CommandBuffer& _commandBuffer, const Device& _device,
                         const CommandPool& _commandPool)
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = _commandPool.commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(_device.device, &allocInfo, &_commandBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}

void destroyCommandBuffer(CommandBuffer& _commandBuffer, const Device& _device,
                          const CommandPool& _commandPool)
{
    vkFreeCommandBuffers(_device.device, _commandPool.commandPool, 1, &_commandBuffer);
}

void beingCommandBuffer(CommandBuffer& _commandBuffer, const Surface& _surface)
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;                  // Optional
    beginInfo.pInheritanceInfo = nullptr; // Optional

    if (vkBeginCommandBuffer(_commandBuffer, &beginInfo) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to begin recording command buffer!");
    }
}

void endCommandBuffer(CommandBuffer& _commandBuffer)
{
    if (vkEndCommandBuffer(_commandBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to record command buffer!");
    }
}

} // namespace vulkan
} // namespace graphics
} // namespace saturn
