#pragma once

#include "vulkan_common.hpp"

#include "context/vulkan_device.hpp"
#include "vulkan_swapchain.hpp"
#include "commands/vulkan_render_pass.hpp"

namespace saturn
{
namespace graphics
{
namespace vulkan
{

void createSwapchainFramebuffers(Swapchain& _swapchain, const Device& _device,
                                 const RenderPass& _renderPass);

void destroySwapchainFramebuffers(Swapchain& _swapchain, const Device& _device);

} // namespace vulkan
} // namespace graphics
} // namespace saturn
