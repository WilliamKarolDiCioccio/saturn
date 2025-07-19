#pragma once

#include "../vulkan_common.hpp"

namespace mosaic
{
namespace graphics
{
namespace vulkan
{

struct Instance
{
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    std::vector<const char*> requiredExtensions;
    std::vector<const char*> optionalExtensions;
    std::vector<const char*> availableExtensions;
    std::vector<const char*> requiredLayers;
    std::vector<const char*> availableLayers;

    Instance() : instance(nullptr), debugMessenger(nullptr)
    {
        requiredExtensions = {
            VK_KHR_SURFACE_EXTENSION_NAME,
#if defined(MOSAIC_PLATFORM_WINDOWS)
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#elif defined(MOSAIC_PLATFORM_ANDROID)
            VK_KHR_ANDROID_SURFACE_EXTENSION_NAME,
#endif
        };

        optionalExtensions = {
            VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME,
        };

#if defined(MOSAIC_DEBUG_BUILD) || defined(MOSAIC_DEV_BUILD)
        requiredExtensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        requiredExtensions.emplace_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);

        requiredLayers = {
            "VK_LAYER_KHRONOS_validation",
        };
#endif
    }
};

void createInstance(Instance& _instance);

void destroyInstance(Instance& _instance);

} // namespace vulkan
} // namespace graphics
} // namespace mosaic
