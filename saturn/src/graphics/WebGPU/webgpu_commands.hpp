#pragma once

#include "webgpu_common.hpp"

namespace saturn
{
namespace graphics
{
namespace webgpu
{

WGPUCommandEncoder createCommandEncoder(WGPUDevice _device, const char* _label);

WGPUCommandBuffer createCommandBuffer(WGPUCommandEncoder _encoder,
                                      WGPUCommandBufferDescriptor _descriptor);

WGPURenderPassEncoder beginRenderPass(WGPUCommandEncoder _encoder, WGPUTextureView _targetView,
                                      WGPUColor _clearColor);

void endRenderPass(WGPURenderPassEncoder _renderPass);

void submitCommands(WGPUQueue _queue, std::vector<WGPUCommandBuffer>& _commands);

} // namespace webgpu
} // namespace graphics
} // namespace saturn
