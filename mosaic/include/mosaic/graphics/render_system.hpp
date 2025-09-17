#pragma once

#include <memory>
#include <unordered_map>

#include <pieces/core/result.hpp>

#include "mosaic/internal/defines.hpp"
#include "mosaic/window/window.hpp"
#include "mosaic/core/logger.hpp"
#include "mosaic/core/system.hpp"

#include "render_context.hpp"

namespace mosaic
{
namespace graphics
{

enum class RendererAPIType
{
    web_gpu,
    vulkan,
    none
};

class MOSAIC_API RenderSystem : public core::EngineSystem
{
   private:
    static RenderSystem* g_instance;

    RendererAPIType m_apiType;
    std::unordered_map<const window::Window*, std::unique_ptr<RenderContext>> m_contexts;

   public:
    RenderSystem(RendererAPIType _apiType)
        : EngineSystem(core::EngineSystemType::render), m_apiType(_apiType)
    {
        assert(!g_instance && "RenderSystem already exists!");
        g_instance = this;
    };

    virtual ~RenderSystem() override { g_instance = nullptr; }

    RenderSystem(const RenderSystem&) = delete;
    RenderSystem& operator=(const RenderSystem&) = delete;
    RenderSystem(RenderSystem&&) = default;
    RenderSystem& operator=(RenderSystem&&) = default;

    static std::unique_ptr<RenderSystem> create(RendererAPIType _type);

   public:
    virtual pieces::RefResult<System, std::string> initialize() override = 0;
    virtual void shutdown() override = 0;

    pieces::Result<RenderContext*, std::string> createContext(const window::Window* _window);
    void destroyContext(const window::Window* _window);

    // createMaterial()
    // createShader()
    // createTexture()
    // createMesh()
    // createRenderPass()

    inline void destroyAllContexts()
    {
        for (auto& [window, context] : m_contexts) context->shutdown();

        m_contexts.clear();
    }

    inline pieces::RefResult<System, std::string> update() override
    {
        for (auto& [window, context] : m_contexts) context->render();

        return pieces::OkRef<System, std::string>(*this);
    }

    inline RenderContext* getContext(const window::Window* _window) const
    {
        if (m_contexts.find(_window) != m_contexts.end()) return m_contexts.at(_window).get();

        return nullptr;
    }

    [[nodiscard]] static RenderSystem* getGlobalRenderSystem()
    {
        if (!g_instance) MOSAIC_ERROR("RenderSystem has not been created yet!");

        return g_instance;
    }
};

} // namespace graphics
} // namespace mosaic
