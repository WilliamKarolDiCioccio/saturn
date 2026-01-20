#pragma once

#include "../context/vulkan_device.hpp"
#include "../vulkan_swapchain.hpp"
#include "vulkan_command_pool.hpp"
#include "vulkan_command_buffer.hpp"

namespace saturn
{
namespace graphics
{
namespace vulkan
{

struct RenderPass
{
    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;

    RenderPass() : renderPass(nullptr), pipelineLayout(nullptr) {};
};

void createRenderPass(RenderPass& _renderPass, const Device& _device, const Swapchain& _swapchain);

void destroyRenderPass(RenderPass& _renderPass, const Device& _device);

void beginRenderPass(RenderPass& _renderPass, const Swapchain& _swapchain,
                     const CommandBuffer& _commandBuffer, uint32_t imageIndex);

void endRenderPass(VkCommandBuffer commandBuffer);

} // namespace vulkan
} // namespace graphics
} // namespace saturn
