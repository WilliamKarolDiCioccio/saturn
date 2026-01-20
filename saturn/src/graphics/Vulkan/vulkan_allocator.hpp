#pragma once

#include <vk_mem_alloc.h>

#include "vulkan_common.hpp"

namespace saturn
{
namespace graphics
{
namespace vulkan
{

void createAllocator(VmaAllocator& _allocator, const VkInstance& _instance,
                     const VkPhysicalDevice& _physicalDevice, const VkDevice& _device);

void destroyAllocator(VmaAllocator& allocator);

} // namespace vulkan
} // namespace graphics
} // namespace saturn
