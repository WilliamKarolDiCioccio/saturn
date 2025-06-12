#include "mosaic/graphics/render_system.hpp"

#ifndef MOSAIC_PLATFORM_ANDROID
#include "WebGPU/webgpu_render_context.hpp"
#include "WebGPU/webgpu_render_system.hpp"
#endif

#ifndef MOSAIC_PLATFORM_EMSCRIPTEN
#include "Vulkan/vulkan_render_context.hpp"
#include "Vulkan/vulkan_render_system.hpp"
#endif

namespace mosaic
{
namespace graphics
{

pieces::Result<RenderContext*, std::string> RenderSystem::createContext(const core::Window* _window)
{
    if (m_contexts.find(_window) != m_contexts.end())
    {
        MOSAIC_WARN("RenderSystem: Context already exists for this window");

        return pieces::Ok<RenderContext*, std::string>(m_contexts[_window].get());
    }

    switch (m_apiType)
    {
#ifndef MOSAIC_PLATFORM_ANDROID
        case RendererAPIType::web_gpu:
        {
            if (m_contexts.size() > 1)
            {
                return pieces::Err<RenderContext*, std::string>(
                    "WebGPU backend only supports one context at a time");
            }

            m_contexts[_window] = std::make_unique<webgpu::WebGPURenderContext>(
                _window, RenderContextSettings(true, 2));

            break;
        }
#endif
#ifndef MOSAIC_PLATFORM_EMSCRIPTEN
        case RendererAPIType::vulkan:
        {
            m_contexts[_window] = std::make_unique<vulkan::VulkanRenderContext>(
                _window, RenderContextSettings(true, 2));

            break;
        }
#endif
        default:
        {
            return pieces::Err<RenderContext*, std::string>("RenderSystem: Unsupported API type");
        }
    }

    auto result = m_contexts[_window]->initialize(this);

    if (result.isErr())
    {
        m_contexts.erase(_window);

        return pieces::Err<RenderContext*, std::string>(std::move(result.error()));
    }

    return pieces::Ok<RenderContext*, std::string>(m_contexts.at(_window).get());
}

void RenderSystem::destroyContext(const core::Window* _window)
{
    auto it = m_contexts.find(_window);

    if (it != m_contexts.end())
    {
        it->second->shutdown();

        m_contexts.erase(it);
    }
}

std::unique_ptr<RenderSystem> RenderSystem::create(RendererAPIType _apiType)
{
    switch (_apiType)
    {
#ifndef MOSAIC_PLATFORM_ANDROID
        case RendererAPIType::web_gpu:
            return std::make_unique<webgpu::WebGPURenderSystem>();
#endif
#ifndef MOSAIC_PLATFORM_EMSCRIPTEN
        case RendererAPIType::vulkan:
            return std::make_unique<vulkan::VulkanRenderSystem>();
#endif
    }
}

} // namespace graphics
} // namespace mosaic
