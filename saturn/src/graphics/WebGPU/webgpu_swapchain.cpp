#include "webgpu_swapchain.hpp"

namespace saturn
{
namespace graphics
{
namespace webgpu
{

void configureSwapchain(WGPUAdapter _adapter, WGPUDevice _device, WGPUSurface _surface,
                        GLFWwindow* _glfwHandle, glm::uvec2 _framebufferExtent)
{
    WGPUSurfaceCapabilities surfaceCapabilities;

    if (wgpuSurfaceGetCapabilities(_surface, _adapter, &surfaceCapabilities) != WGPUStatus_Success)
    {
        SATURN_ERROR("Could not get WebGPU surface capabilities!");
        return;
    }

    if (!std::find(surfaceCapabilities.formats,
                   surfaceCapabilities.formats + surfaceCapabilities.formatCount,
                   WGPUTextureFormat_RGBA8UnormSrgb))
    {
        SATURN_ERROR("WebGPU surface does not support RGBA8UnormSrgb!");
        return;
    }

    if (!(surfaceCapabilities.usages & WGPUTextureUsage_RenderAttachment))
    {
        SATURN_ERROR("Surface texture does not support RenderAttachment usage!");
        return;
    }

    WGPUSurfaceConfiguration surfaceConfig = {};
    surfaceConfig.nextInChain = nullptr;
    surfaceConfig.width = _framebufferExtent.x;
    surfaceConfig.height = _framebufferExtent.y;
    surfaceConfig.format = WGPUTextureFormat_RGBA8UnormSrgb;
    surfaceConfig.usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_TextureBinding;
    surfaceConfig.device = _device;
    surfaceConfig.presentMode = WGPUPresentMode_Fifo;
    surfaceConfig.alphaMode = WGPUCompositeAlphaMode_Auto;
    surfaceConfig.viewFormatCount = 0;
    surfaceConfig.viewFormats = nullptr;

    wgpuSurfaceConfigure(_surface, &surfaceConfig);
}

} // namespace webgpu
} // namespace graphics
} // namespace saturn
