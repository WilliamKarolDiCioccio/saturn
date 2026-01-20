#pragma once

#include "../context/vulkan_device.hpp"
#include "vulkan_command_pool.hpp"

namespace saturn
{
namespace graphics
{
namespace vulkan
{

using CommandBuffer = VkCommandBuffer;

void createCommandBuffer(CommandBuffer& _commandBuffer, const Device& _device,
                         const CommandPool& _commandPool);

void destroyCommandBuffer(CommandBuffer& _commandBuffer, const Device& _device,
                          const CommandPool& _commandPool);

void beingCommandBuffer(CommandBuffer& _commandPool, const Surface& _surface);

void endCommandBuffer(CommandBuffer& _commandPool);

} // namespace vulkan
} // namespace graphics
} // namespace saturn
