#pragma once

#include "../context/vulkan_device.hpp"

namespace saturn
{
namespace graphics
{
namespace vulkan
{

struct CommandPool
{
    VkCommandPool commandPool;

    CommandPool() : commandPool(VK_NULL_HANDLE){};
};

void createCommandPool(CommandPool& _commandPool, const Device& _device, const Surface& _surface);

void destroyCommandPool(CommandPool& _commandPool, const Device& _device);

} // namespace vulkan
} // namespace graphics
} // namespace saturn
