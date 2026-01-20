#include "vulkan_command_pool.hpp"

namespace saturn
{
namespace graphics
{
namespace vulkan
{

void createCommandPool(CommandPool& _commandPool, const Device& _device, const Surface& _surface)
{
    QueueFamilySupportDetails queueFamilyIndices =
        findDeviceQueueFamiliesSupport(_device.physicalDevice, _surface.surface);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

    if (vkCreateCommandPool(_device.device, &poolInfo, nullptr, &_commandPool.commandPool) !=
        VK_SUCCESS)
    {
        throw std::runtime_error("failed to create command pool!");
    }
}

void destroyCommandPool(CommandPool& _commandPool, const Device& _device)
{
    vkDestroyCommandPool(_device.device, _commandPool.commandPool, nullptr);
}

} // namespace vulkan
} // namespace graphics
} // namespace saturn
