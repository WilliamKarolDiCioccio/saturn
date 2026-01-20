#pragma once

#include "saturn/tools/logger.hpp"
#include "saturn/window/window.hpp"

#include <volk.h>
#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

#if defined(SATURN_PLATFORM_WINDOWS)
#include <windows.h>
#include <vulkan/vulkan_win32.h>
#elif defined(SATURN_PLATFORM_ANDROID)
#include <vulkan/vulkan_android.h>
#endif

namespace saturn
{
namespace graphics
{
namespace vulkan
{

static void checkVkResult(VkResult _result)
{
    auto resultStr = std::string(string_VkResult(_result));

    switch (_result)
    {
        case VK_SUCCESS:
            SATURN_DEBUG("Vulkan result: VK_SUCCESS");
            break;
        case VK_NOT_READY:
        case VK_TIMEOUT:
        case VK_EVENT_SET:
        case VK_EVENT_RESET:
        case VK_INCOMPLETE:
            SATURN_WARN("Vulkan result: {}", resultStr);
            break;
        case VK_ERROR_OUT_OF_HOST_MEMORY:
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
        case VK_ERROR_INITIALIZATION_FAILED:
        case VK_ERROR_DEVICE_LOST:
        case VK_ERROR_MEMORY_MAP_FAILED:
        case VK_ERROR_LAYER_NOT_PRESENT:
        case VK_ERROR_EXTENSION_NOT_PRESENT:
        case VK_ERROR_FEATURE_NOT_PRESENT:
        case VK_ERROR_INCOMPATIBLE_DRIVER:
        case VK_ERROR_TOO_MANY_OBJECTS:
        case VK_ERROR_FORMAT_NOT_SUPPORTED:
        case VK_ERROR_FRAGMENTED_POOL:
        case VK_ERROR_UNKNOWN:
        case VK_ERROR_OUT_OF_POOL_MEMORY:
        case VK_ERROR_INVALID_EXTERNAL_HANDLE:
        case VK_ERROR_FRAGMENTATION:
        case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
        case VK_PIPELINE_COMPILE_REQUIRED:
        case VK_ERROR_SURFACE_LOST_KHR:
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
        case VK_SUBOPTIMAL_KHR:
        case VK_ERROR_OUT_OF_DATE_KHR:
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
        case VK_ERROR_VALIDATION_FAILED_EXT:
        case VK_ERROR_INVALID_SHADER_NV:
        case VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR:
        case VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR:
        case VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR:
        case VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR:
        case VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR:
        case VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR:
        case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT:
        case VK_ERROR_NOT_PERMITTED_KHR:
        case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
        case VK_THREAD_IDLE_KHR:
        case VK_THREAD_DONE_KHR:
        case VK_OPERATION_DEFERRED_KHR:
        case VK_OPERATION_NOT_DEFERRED_KHR:
        case VK_ERROR_COMPRESSION_EXHAUSTED_EXT:
        case VK_ERROR_INCOMPATIBLE_SHADER_BINARY_EXT:
        case VK_RESULT_MAX_ENUM:
            SATURN_ERROR("Vulkan result: {}", resultStr);
            break;
        default:
            SATURN_CRITICAL("Unknown Vulkan result: {}", resultStr);
            break;
    }

    assert(_result == VK_SUCCESS);
}

} // namespace vulkan
} // namespace graphics
} // namespace saturn
