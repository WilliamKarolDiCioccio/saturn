#include "vulkan_swapchain.hpp"

#include <algorithm>

#if defined(SATURN_PLATFORM_DESKTOP) || defined(SATURN_PLATFORM_WEB)
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#endif

namespace saturn
{
namespace graphics
{
namespace vulkan
{

#ifdef SATURN_PLATFORM_WINDOWS

VkResult acquireExclusiveFullscreenMode(Swapchain& _swapchain)
{
    PFN_vkAcquireFullScreenExclusiveModeEXT vkAcquireFullScreenExclusiveModeEXT =
        (PFN_vkAcquireFullScreenExclusiveModeEXT)vkGetDeviceProcAddr(
            _swapchain.device, "vkAcquireFullScreenExclusiveModeEXT");

    vkAcquireFullScreenExclusiveModeEXT(_swapchain.device, _swapchain.swapchain);

    return VK_SUCCESS;
}

VkResult releaseExclusiveFullscreenMode(Swapchain& _swapchain)
{
    PFN_vkReleaseFullScreenExclusiveModeEXT vkReleaseFullScreenExclusiveModeEXT =
        (PFN_vkReleaseFullScreenExclusiveModeEXT)vkGetDeviceProcAddr(
            _swapchain.device, "vkReleaseFullScreenExclusiveModeEXT");

    vkReleaseFullScreenExclusiveModeEXT(_swapchain.device, _swapchain.swapchain);

    return VK_SUCCESS;
}

#endif

void createImageViews(Swapchain& _swapchain)
{
    _swapchain.imageViews.resize(_swapchain.images.size());

    for (size_t i = 0; i < _swapchain.images.size(); i++)
    {
        VkImageViewCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.flags = 0;
        createInfo.image = _swapchain.images[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = _swapchain.surfaceFormat.format;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(_swapchain.device, &createInfo, nullptr, &_swapchain.imageViews[i]) !=
            VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create Vulkan image views!");
        }
    }
}

void destroyImageViews(Swapchain& _swapchain)
{
    for (size_t i = 0; i < _swapchain.imageViews.size(); i++)
    {
        vkDestroyImageView(_swapchain.device, _swapchain.imageViews[i], nullptr);
    }
}

VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& _availableFormats)
{
    std::vector<VkFormat> preferredFormats;

#if defined(SATURN_PLATFORM_ANDROID)
    preferredFormats = {
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_FORMAT_B8G8R8A8_UNORM,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_FORMAT_B8G8R8A8_SRGB,
    };
#else
    preferredFormats = {
        VK_FORMAT_B8G8R8A8_SRGB,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_FORMAT_B8G8R8A8_UNORM,
        VK_FORMAT_R8G8B8A8_UNORM,
    };
#endif

    for (const auto& format : preferredFormats)
    {
        auto it = std::find_if(_availableFormats.begin(), _availableFormats.end(),
                               [format](const VkSurfaceFormatKHR& available)
                               {
                                   return available.format == format &&
                                          available.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
                               });

        if (it != _availableFormats.end()) return *it;
    }

    for (const auto& format : preferredFormats)
    {
        auto it = std::find_if(_availableFormats.begin(), _availableFormats.end(),
                               [format](const VkSurfaceFormatKHR& available)
                               { return available.format == format; });

        if (it != _availableFormats.end()) return *it;
    }

    throw std::runtime_error(
        "Failed to find a suitable Vulkan surface format! No preferred formats available.");
}

VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& _availablePresentModes)
{
    if (std::find(_availablePresentModes.begin(), _availablePresentModes.end(),
                  VK_PRESENT_MODE_MAILBOX_KHR) != _availablePresentModes.end())
    {
        return VK_PRESENT_MODE_MAILBOX_KHR;
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& _capabilities,
                            const glm::uvec2& _renderTargetSize)
{
    if (_capabilities.currentExtent.width != UINT32_MAX)
    {
        return _capabilities.currentExtent;
    }
    else
    {
        VkExtent2D actualExtent = {_renderTargetSize.x, _renderTargetSize.y};

        actualExtent.width = std::clamp(actualExtent.width, _capabilities.minImageExtent.width,
                                        _capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, _capabilities.minImageExtent.height,
                                         _capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

void createSwapchain(Swapchain& _swapchain, const Device& _device, const Surface& _surface,
                     [[maybe_unused]] void* _nativeWindowHandle, glm::uvec2 _framebufferExtent,
                     [[maybe_unused]] bool _exclusiveFullscreenRequestable)
{
    _swapchain.device = _device.device;

    SwapChainSupportDetails swapChainSupport =
        findDeviceSwapChainSupport(_device.physicalDevice, _surface.surface);

    _swapchain.surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    _swapchain.presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    _swapchain.extent = chooseSwapExtent(swapChainSupport.capabilities, _framebufferExtent);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

    if (swapChainSupport.capabilities.maxImageCount > 0 &&
        imageCount > swapChainSupport.capabilities.maxImageCount)
    {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;

    void* headExtPtr = nullptr;

#ifdef SATURN_PLATFORM_WINDOWS

    _swapchain.exclusiveFullscreenAvailable =
        std::find(_device.availableExtensions.begin(), _device.availableExtensions.end(),
                  VK_EXT_FULL_SCREEN_EXCLUSIVE_EXTENSION_NAME) != _device.availableExtensions.end();

    VkSurfaceFullScreenExclusiveInfoEXT fullScreenExclusiveInfo = {};
    fullScreenExclusiveInfo.sType = VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_INFO_EXT;
    fullScreenExclusiveInfo.fullScreenExclusive =
        VK_FULL_SCREEN_EXCLUSIVE_APPLICATION_CONTROLLED_EXT;

    HWND win32Handle = glfwGetWin32Window(static_cast<GLFWwindow*>(_nativeWindowHandle));
    HMONITOR monitor = MonitorFromWindow(win32Handle, MONITOR_DEFAULTTONEAREST);

    VkSurfaceFullScreenExclusiveWin32InfoEXT win32FullScreenExclusiveInfo = {
        .sType = VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_WIN32_INFO_EXT,
        .pNext = nullptr,
        .hmonitor = monitor,
    };

    fullScreenExclusiveInfo.pNext = &win32FullScreenExclusiveInfo;

    if (_exclusiveFullscreenRequestable && _swapchain.exclusiveFullscreenAvailable)
    {
        headExtPtr = (void*)&fullScreenExclusiveInfo;
    }

#endif

    createInfo.pNext = headExtPtr;

    createInfo.flags = 0;
    createInfo.surface = _surface.surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = _swapchain.surfaceFormat.format;
    createInfo.imageColorSpace = _swapchain.surfaceFormat.colorSpace;
    createInfo.imageExtent = _swapchain.extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.presentMode = _swapchain.presentMode;

    QueueFamilySupportDetails queueSupportDetails =
        findDeviceQueueFamiliesSupport(_device.physicalDevice, _surface.surface);

    uint32_t queueFamilyIndices[] = {
        queueSupportDetails.graphicsFamily.value(),
        queueSupportDetails.presentFamily.value(),
    };

    if (queueSupportDetails.graphicsFamily != queueSupportDetails.presentFamily)
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = nullptr;

    SATURN_INFO("Surface dimensions: {}x{}", _swapchain.extent.width, _swapchain.extent.height);

    if (vkCreateSwapchainKHR(_swapchain.device, &createInfo, nullptr, &_swapchain.swapchain) !=
        VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Vulkan swapchain!");
    }

    vkGetSwapchainImagesKHR(_swapchain.device, _swapchain.swapchain, &imageCount, nullptr);
    _swapchain.images.resize(imageCount);
    vkGetSwapchainImagesKHR(_swapchain.device, _swapchain.swapchain, &imageCount,
                            _swapchain.images.data());

    createImageViews(_swapchain);

#ifdef SATURN_PLATFORM_WINDOWS
    if (_swapchain.exclusiveFullscreenAvailable) acquireExclusiveFullscreenMode(_swapchain);
#endif
}

void destroySwapchain(Swapchain& _swapchain)
{
#ifdef SATURN_PLATFORM_WINDOWS
    if (_swapchain.exclusiveFullscreenAvailable) releaseExclusiveFullscreenMode(_swapchain);
#endif

    destroyImageViews(_swapchain);
    vkDestroySwapchainKHR(_swapchain.device, _swapchain.swapchain, nullptr);
}

} // namespace vulkan
} // namespace graphics
} // namespace saturn
