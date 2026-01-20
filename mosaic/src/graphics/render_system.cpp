#include "saturn/graphics/render_system.hpp"

#include <cassert>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include <pieces/core/result.hpp>

#include "saturn/tools/logger.hpp"
#include "saturn/core/system.hpp"
#include "saturn/graphics/render_context.hpp"
#include "saturn/window/window.hpp"
#include "saturn/defines.hpp"

#ifndef SATURN_PLATFORM_ANDROID
#include "WebGPU/webgpu_render_context.hpp"
#include "WebGPU/webgpu_render_system.hpp"
#endif

#ifndef SATURN_PLATFORM_EMSCRIPTEN
#include "Vulkan/vulkan_render_context.hpp"
#include "Vulkan/vulkan_render_system.hpp"
#endif

namespace saturn
{
namespace graphics
{

struct RenderSystem::Impl
{
    RendererAPIType apiType;
    std::unordered_map<const window::Window*, std::unique_ptr<RenderContext>> contexts;

    Impl(RendererAPIType _apiType) : apiType(_apiType) {}
};

RenderSystem* RenderSystem::g_instance = nullptr;

RenderSystem::RenderSystem(RendererAPIType _apiType)
    : EngineSystem(core::EngineSystemType::render), m_impl(new Impl(_apiType))
{
    assert(!g_instance && "RenderSystem already exists!");
    g_instance = this;
};

RenderSystem::~RenderSystem()
{
    g_instance = nullptr;
    delete m_impl;
}

std::unique_ptr<RenderSystem> RenderSystem::create(RendererAPIType _apiType)
{
    switch (_apiType)
    {
        case RendererAPIType::web_gpu:
#ifndef SATURN_PLATFORM_ANDROID
            return std::make_unique<webgpu::WebGPURenderSystem>();
#else
            throw std::runtime_error("WebGPU backend is not supported on Android platform");
#endif
        case RendererAPIType::vulkan:
#ifndef SATURN_PLATFORM_EMSCRIPTEN
            return std::make_unique<vulkan::VulkanRenderSystem>();
#else
            throw std::runtime_error("Vulkan backend is not supported on Emscripten platform");
#endif
    }
}

pieces::Result<RenderContext*, std::string> RenderSystem::createContext(
    const window::Window* _window)
{
    auto& contexts = m_impl->contexts;

    if (contexts.find(_window) != contexts.end())
    {
        SATURN_WARN("RenderSystem: Context already exists for this window");

        return pieces::Ok<RenderContext*, std::string>(contexts[_window].get());
    }

    switch (m_impl->apiType)
    {
#ifndef SATURN_PLATFORM_ANDROID
        case RendererAPIType::web_gpu:
        {
            if (contexts.size() > 1)
            {
                return pieces::Err<RenderContext*, std::string>(
                    "WebGPU backend only supports one context at a time");
            }

            contexts[_window] = std::make_unique<webgpu::WebGPURenderContext>(
                _window, RenderContextSettings(true, 2));

            break;
        }
#endif
#ifndef SATURN_PLATFORM_EMSCRIPTEN
        case RendererAPIType::vulkan:
        {
            contexts[_window] = std::make_unique<vulkan::VulkanRenderContext>(
                _window, RenderContextSettings(true, 2));

            break;
        }
#endif
        default:
        {
            return pieces::Err<RenderContext*, std::string>("RenderSystem: Unsupported API type");
        }
    }

    auto result = contexts[_window]->initialize(this);

    if (result.isErr())
    {
        contexts.erase(_window);

        return pieces::Err<RenderContext*, std::string>(std::move(result.error()));
    }

    return pieces::Ok<RenderContext*, std::string>(contexts.at(_window).get());
}

void RenderSystem::destroyContext(const window::Window* _window)
{
    auto it = m_impl->contexts.find(_window);

    if (it != m_impl->contexts.end())
    {
        it->second->shutdown();

        m_impl->contexts.erase(it);
    }
}

void RenderSystem::destroyAllContexts()
{
    for (auto& [window, context] : m_impl->contexts) context->shutdown();

    m_impl->contexts.clear();
}

pieces::RefResult<core::System, std::string> RenderSystem::update()
{
    for (auto& [window, context] : m_impl->contexts) context->render();

    return pieces::OkRef<core::System, std::string>(*this);
}

RenderContext* RenderSystem::getContext(const window::Window* _window) const
{
    if (m_impl->contexts.find(_window) != m_impl->contexts.end())
    {
        return m_impl->contexts.at(_window).get();
    }

    return nullptr;
}

} // namespace graphics
} // namespace saturn
