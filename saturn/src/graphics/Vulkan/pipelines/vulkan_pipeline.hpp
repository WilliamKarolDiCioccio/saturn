#pragma once

#include "../context/vulkan_device.hpp"
#include "../vulkan_swapchain.hpp"
#include "../commands/vulkan_render_pass.hpp"
#include "vulkan_shader_module.hpp"

namespace saturn
{
namespace graphics
{
namespace vulkan
{

struct Pipeline
{
    VkDevice device;
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;

    Pipeline() : device(VK_NULL_HANDLE), pipeline(VK_NULL_HANDLE), pipelineLayout(VK_NULL_HANDLE) {}
};

void createGraphicsPipeline(Pipeline& _pipeline, const Device& _device, const Swapchain& _swapchain,
                            const RenderPass& _renderPass);

void destroyGraphicsPipeline(Pipeline& _pipeline, const Device& _device);

void bindGraphicsPipeline(const Pipeline& _pipeline, const CommandBuffer& _commandBuffer);

} // namespace vulkan
} // namespace graphics
} // namespace saturn
