#include "webgpu_render_context.hpp"

#include <vector>

#include <GLFW/glfw3.h>

#include "webgpu_device.hpp"
#include "webgpu_instance.hpp"
#include "webgpu_commands.hpp"
#include "webgpu_swapchain.hpp"

namespace mosaic
{
namespace graphics
{
namespace webgpu
{

WebGPURenderContext::WebGPURenderContext(const window::Window* _window,
                                         const RenderContextSettings& _settings)
    : m_instance(nullptr),
      m_surface(nullptr),
      m_adapter(nullptr),
      m_device(nullptr),
      m_presentQueue(nullptr),
      RenderContext(_window, _settings) {};

pieces::RefResult<RenderContext, std::string> WebGPURenderContext::initialize(
    RenderSystem* _renderSystem)
{
    auto window = getWindow();

    m_surface = glfwCreateWindowWGPUSurface(m_instance,
                                            static_cast<GLFWwindow*>(window->getNativeHandle()));

    m_instance = createInstance();

    m_adapter = requestAdapter(m_instance, m_surface);

    if (!isAdapterSuitable(m_adapter))
    {
        return pieces::ErrRef<RenderContext, std::string>("WebGPU adapter is not suitable!");
    }

    m_device = createDevice(m_adapter);

    m_presentQueue = wgpuDeviceGetQueue(m_device);

    WGPUQueueWorkDoneCallback onQueueWorkDone =
        [](WGPUQueueWorkDoneStatus status, void* userData1, void* userData2)
    {
        switch (status)
        {
            case WGPUQueueWorkDoneStatus_Success:
                MOSAIC_INFO("WebGPU queue work done successfully!");
                break;
            case WGPUQueueWorkDoneStatus_Error:
                MOSAIC_ERROR("WebGPU queue work done with error!");
                break;
#if defined(WEBGPU_BACKEND_WGPU)
            case WGPUQueueWorkDoneStatus_Unknown:
                MOSAIC_ERROR("WebGPU queue work done with unknown status!");
                break;
#endif
            default:
                MOSAIC_ERROR("WebGPU queue work done with default status!");
                break;
        }
    };

    WGPUQueueWorkDoneCallbackInfo workDoneCallbackInfo = {
        .callback = onQueueWorkDone,
        .userdata1 = nullptr,
        .userdata2 = nullptr,
    };

    wgpuQueueOnSubmittedWorkDone(m_presentQueue, workDoneCallbackInfo);

    configureSwapchain(m_adapter, m_device, m_surface,
                       static_cast<GLFWwindow*>(window->getNativeHandle()),
                       window->getFramebufferSize());

    wgpuAdapterRelease(m_adapter);

    return pieces::OkRef<RenderContext, std::string>(*this);
}

void WebGPURenderContext::shutdown()
{
    wgpuSurfaceUnconfigure(m_surface);
    wgpuSurfaceRelease(m_surface);
    wgpuInstanceRelease(m_instance);
    wgpuQueueRelease(m_presentQueue);
    wgpuDeviceRelease(m_device);
}

void WebGPURenderContext::resizeFramebuffer() {}

void mosaic::graphics::webgpu::WebGPURenderContext::recreateSurface() {}

void WebGPURenderContext::getNextSurfaceViewData()
{
    WGPUSurfaceTexture surfaceTexture;

    wgpuSurfaceGetCurrentTexture(m_surface, &surfaceTexture);

    if (surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal)
    {
        m_frameData.targetView = nullptr;
        m_frameData.surfaceTexture = {};
    }

    WGPUTextureViewDescriptor viewDescriptor;
    viewDescriptor.nextInChain = nullptr;
    viewDescriptor.label = WGPUStringView("Surface texture view", 21);
    viewDescriptor.format = wgpuTextureGetFormat(surfaceTexture.texture);
    viewDescriptor.dimension = WGPUTextureViewDimension_2D;
    viewDescriptor.baseMipLevel = 0;
    viewDescriptor.mipLevelCount = 1;
    viewDescriptor.baseArrayLayer = 0;
    viewDescriptor.arrayLayerCount = 1;
    viewDescriptor.aspect = WGPUTextureAspect_All;
    viewDescriptor.usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_TextureBinding;

    WGPUTextureView targetView = wgpuTextureCreateView(surfaceTexture.texture, &viewDescriptor);

#ifndef WEBGPU_BACKEND_WGPU
    wgpuTextureRelease(surfaceTexture.texture);
#endif

    m_frameData.surfaceTexture = surfaceTexture;
    m_frameData.targetView = targetView;
}

void WebGPURenderContext::pollDevice(int _times)
{
    for (int i = 0; i < _times; ++i)
    {
#if defined(WEBGPU_BACKEND_DAWN)
        wgpuDeviceTick(m_device);
#elif defined(WEBGPU_BACKEND_WGPU)
        wgpuDevicePoll(m_device, false, nullptr);
#endif
    }
}

void WebGPURenderContext::beginFrame()
{
    getNextSurfaceViewData();

    m_frameData.commandEncoder = createCommandEncoder(m_device, "Clear Screen Encoder");

    m_frameData.renderPass = beginRenderPass(m_frameData.commandEncoder, m_frameData.targetView,
                                             {
                                                 0.25f,
                                                 0.1f,
                                                 0.5f,
                                                 1.0f,
                                             });
}

void WebGPURenderContext::updateResources()
{
    // Placeholder for resource updates (uniforms, buffer data, etc.)
}

void WebGPURenderContext::drawScene()
{
    // This is where draw calls would go
    // e.g., wgpuRenderPassEncoderDraw(...);

    endRenderPass(m_frameData.renderPass);
}

void WebGPURenderContext::endFrame()
{
    std::vector<WGPUCommandBuffer> commands;

    WGPUCommandBufferDescriptor cmdBufferDescriptor = {};
    cmdBufferDescriptor.nextInChain = nullptr;
    cmdBufferDescriptor.label = WGPUStringView("Command buffer", 15);

    WGPUCommandBuffer command =
        createCommandBuffer(m_frameData.commandEncoder, cmdBufferDescriptor);
    commands.push_back(command);

    submitCommands(m_presentQueue, commands);
    pollDevice();

    wgpuTextureViewRelease(m_frameData.targetView);

#ifndef __EMSCRIPTEN__
    wgpuSurfacePresent(m_surface);
#endif

#ifdef WEBGPU_BACKEND_WGPU
    wgpuTextureRelease(m_frameData.surfaceTexture.texture);
#endif
}

} // namespace webgpu
} // namespace graphics
} // namespace mosaic
