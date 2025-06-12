#include "vulkan_surface.hpp"

#if defined(MOSAIC_PLATFORM_DESKTOP) || defined(MOSAIC_PLATFORM_WEB)
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#elif defined(MOSAIC_PLATFORM_ANDROID)
#include <android/native_window.h>
#endif

namespace mosaic
{
namespace graphics
{
namespace vulkan
{

#if defined(MOSAIC_PLATFORM_WINDOWS)

VkResult createWin32Surface(VkInstance instance, const VkWin32SurfaceCreateInfoKHR* pCreateInfo,
                            const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface)
{
    auto func = reinterpret_cast<PFN_vkCreateWin32SurfaceKHR>(
        vkGetInstanceProcAddr(instance, "vkCreateWin32SurfaceKHR"));

    if (func != nullptr)
    {
        return func(instance, pCreateInfo, pAllocator, pSurface);
    }
    else
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

#elif defined(MOSAIC_PLATFORM_ANDROID)

VkResult createAndroidSurface(VkInstance instance, const VkAndroidSurfaceCreateInfoKHR* pCreateInfo,
                              const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface)
{
    auto func = reinterpret_cast<PFN_vkCreateAndroidSurfaceKHR>(
        vkGetInstanceProcAddr(instance, "vkCreateAndroidSurfaceKHR"));

    if (func != nullptr)
    {
        return func(instance, pCreateInfo, pAllocator, pSurface);
    }
    else
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

#endif

void createSurface(Surface& _surface, const Instance& _instance, void* _nativeWindowHandle)
{
#if defined(MOSAIC_PLATFORM_WINDOWS)

    const VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .pNext = nullptr,
        .hinstance = GetModuleHandle(nullptr),
        .hwnd = glfwGetWin32Window(static_cast<GLFWwindow*>(_nativeWindowHandle)),
    };

    if (createWin32Surface(_instance.instance, &surfaceCreateInfo, nullptr, &_surface.surface) !=
        VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Win32 Vulkan surface!");
    }

#elif defined(MOSAIC_PLATFORM_ANDROID)

    const VkAndroidSurfaceCreateInfoKHR surfaceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,
        .pNext = nullptr,
        .flags = 0,
        .window = static_cast<ANativeWindow*>(_nativeWindowHandle),
    };

    if (createAndroidSurface(_instance.instance, &surfaceCreateInfo, nullptr, &_surface.surface) !=
        VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Android Vulkan surface!");
    }

#endif
}

void destroySurface(Surface& _surface, const Instance& _instance)
{
    vkDestroySurfaceKHR(_instance.instance, _surface.surface, nullptr);
}

} // namespace vulkan
} // namespace graphics
} // namespace mosaic
