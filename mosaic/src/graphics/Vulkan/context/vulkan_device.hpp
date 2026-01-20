#pragma once

#include "../vulkan_common.hpp"
#include "vulkan_instance.hpp"
#include "vulkan_surface.hpp"

namespace saturn
{
namespace graphics
{
namespace vulkan
{

struct QueueFamilySupportDetails
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() { return graphicsFamily.has_value() && presentFamily.has_value(); }
};

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;

    SwapChainSupportDetails() : capabilities{}, formats{}, presentModes{} {};

    bool isComplete() { return !formats.empty() && !presentModes.empty(); }
};

struct Device
{
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    std::vector<const char*> requiredExtensions;
    std::vector<const char*> optionalExtensions;
    std::vector<const char*> availableExtensions;
    std::vector<const char*> requiredLayers;
    std::vector<const char*> availableLayers;

    Device()
        : physicalDevice(nullptr), device(nullptr), graphicsQueue(nullptr), presentQueue(nullptr)
    {
        requiredExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#if defined(SATURN_PLATFORM_DESKTOP)
            VK_KHR_MULTIVIEW_EXTENSION_NAME,
#endif
        };

        optionalExtensions = {
#ifdef SATURN_PLATFORM_WINDOWS
            VK_EXT_FULL_SCREEN_EXCLUSIVE_EXTENSION_NAME,
#endif
        };

#if defined(SATURN_DEBUG_BUILD) || defined(SATURN_DEV_BUILD)
        requiredLayers = {
            "VK_LAYER_KHRONOS_validation",
        };
#endif
    }
};

void createDevice(Device& _device, const Instance& _instance, const Surface& _surface);

void destroyDevice(Device& _device);

QueueFamilySupportDetails findDeviceQueueFamiliesSupport(const VkPhysicalDevice& _device,
                                                         const VkSurfaceKHR& _surface);

SwapChainSupportDetails findDeviceSwapChainSupport(const VkPhysicalDevice& device,
                                                   const VkSurfaceKHR& _surface);
} // namespace vulkan
} // namespace graphics
} // namespace saturn
