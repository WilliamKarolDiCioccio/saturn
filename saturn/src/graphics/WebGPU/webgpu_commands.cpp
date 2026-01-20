#include "webgpu_commands.hpp"

namespace saturn
{
namespace graphics
{
namespace webgpu
{

WGPUCommandEncoder createCommandEncoder(WGPUDevice _device, const char* _label)
{
    WGPUCommandEncoderDescriptor encoderDesc = {};
    encoderDesc.nextInChain = nullptr;
    encoderDesc.label = WGPUStringView(_label, strnlen(_label, 32));

    return wgpuDeviceCreateCommandEncoder(_device, &encoderDesc);
}

WGPUCommandBuffer createCommandBuffer(WGPUCommandEncoder _encoder,
                                      WGPUCommandBufferDescriptor _descriptor)
{
    WGPUCommandBuffer commandBuffer = wgpuCommandEncoderFinish(_encoder, &_descriptor);

    wgpuCommandEncoderRelease(_encoder);

    return commandBuffer;
}

WGPURenderPassEncoder beginRenderPass(WGPUCommandEncoder _encoder, WGPUTextureView _targetView,
                                      WGPUColor _clearColor)
{
    WGPURenderPassColorAttachment colorAttachment = {};
    colorAttachment.view = _targetView;
    colorAttachment.resolveTarget = nullptr;
    colorAttachment.loadOp = WGPULoadOp_Clear;
    colorAttachment.storeOp = WGPUStoreOp_Store;
    colorAttachment.clearValue = _clearColor;

#ifndef WEBGPU_BACKEND_WGPU
    colorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
#endif

    WGPURenderPassDescriptor renderPassDesc = {};
    renderPassDesc.nextInChain = nullptr;
    renderPassDesc.colorAttachmentCount = 1;
    renderPassDesc.colorAttachments = &colorAttachment;
    renderPassDesc.depthStencilAttachment = nullptr;
    renderPassDesc.timestampWrites = nullptr;

    return wgpuCommandEncoderBeginRenderPass(_encoder, &renderPassDesc);
}

void endRenderPass(WGPURenderPassEncoder _renderPass)
{
    wgpuRenderPassEncoderEnd(_renderPass);
    wgpuRenderPassEncoderRelease(_renderPass);
}

void submitCommands(WGPUQueue _queue, std::vector<WGPUCommandBuffer>& _commands)
{
    wgpuQueueSubmit(_queue, _commands.size(), _commands.data());

    for (auto cmd : _commands)
    {
        wgpuCommandBufferRelease(cmd);
    }

    _commands.clear();
}

} // namespace webgpu
} // namespace graphics
} // namespace saturn
