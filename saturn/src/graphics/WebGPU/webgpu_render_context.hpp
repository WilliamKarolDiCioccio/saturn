#pragma once

#include "saturn/graphics/render_context.hpp"

#include "webgpu_common.hpp"

namespace saturn
{
namespace graphics
{
namespace webgpu
{

class WebGPURenderContext : public RenderContext
{
   private:
    struct FrameData
    {
        WGPUSurfaceTexture surfaceTexture;
        WGPUTextureView targetView;
        WGPURenderPassEncoder renderPass;
        WGPUCommandEncoder commandEncoder;

        FrameData()
            : surfaceTexture({}),
              targetView(nullptr),
              renderPass(nullptr),
              commandEncoder(nullptr) {};
    } m_frameData;

    WGPUInstance m_instance;
    WGPUSurface m_surface;
    WGPUAdapter m_adapter;
    WGPUDevice m_device;
    WGPUQueue m_presentQueue;

   public:
    WebGPURenderContext(const window::Window* _window, const RenderContextSettings& _settings);
    ~WebGPURenderContext() override = default;

   public:
    pieces::RefResult<RenderContext, std::string> initialize(RenderSystem* _renderSystem) override;
    void shutdown() override;

   private:
    void resizeFramebuffer() override;
    void recreateSurface() override;
    void beginFrame() override;
    void updateResources() override;
    void drawScene() override;
    void endFrame() override;

   private:
    void getNextSurfaceViewData();
    void pollDevice(int _times = 5);
};

} // namespace webgpu
} // namespace graphics
} // namespace saturn
