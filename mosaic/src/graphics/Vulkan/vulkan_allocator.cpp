#include "vulkan_allocator.hpp"

namespace saturn
{
namespace graphics
{
namespace vulkan
{

void createAllocator(VmaAllocator& _allocator, const VkInstance& _instance,
                     const VkPhysicalDevice& _physicalDevice, const VkDevice& _device)
{
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.instance = _instance;
    allocatorInfo.physicalDevice = _physicalDevice;
    allocatorInfo.device = _device;

    if (vmaCreateAllocator(&allocatorInfo, &_allocator) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Vulkan allocator");
    }
}

void destroyAllocator(VmaAllocator& _allocator) { vmaDestroyAllocator(_allocator); }

} // namespace vulkan
} // namespace graphics
} // namespace saturn
