#pragma once

#include <vector>

#include <glm/vec2.hpp>

#include "vulkan_common.hpp"
#include "context/vulkan_device.hpp"

namespace saturn
{
namespace graphics
{
namespace vulkan
{

struct Swapchain
{
    VkDevice device;
    VkSwapchainKHR swapchain;
    VkSurfaceFormatKHR surfaceFormat;
    VkPresentModeKHR presentMode;
    VkExtent2D extent;
    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;
    std::vector<VkFramebuffer> framebuffers;
    bool exclusiveFullscreenAvailable;

    Swapchain()
        : device(nullptr),
          swapchain(nullptr),
          surfaceFormat({}),
          presentMode(),
          extent({}),
          exclusiveFullscreenAvailable(false) {};
};

void createSwapchain(Swapchain& _swapchain, const Device& _device, const Surface& _surface,
                     void* _nativeWindowHandle, glm::uvec2 _framebufferExtent,
                     bool _exclusiveFullscreenRequestable);

void destroySwapchain(Swapchain& _swapchain);

} // namespace vulkan
} // namespace graphics
} // namespace saturn
