#pragma once

#include "../vulkan_common.hpp"

#include "vulkan_instance.hpp"

namespace saturn
{
namespace graphics
{
namespace vulkan
{

struct Surface
{
    VkSurfaceKHR surface;

    Surface() : surface(nullptr) {}
};

void createSurface(Surface& _surface, const Instance& _instance, void* _nativeWindowHandle);

void destroySurface(Surface& _surface, const Instance& _instance);

} // namespace vulkan
} // namespace graphics
} // namespace saturn
