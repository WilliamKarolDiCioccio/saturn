#pragma once

#include <memory>

#include <pieces/core/result.hpp>

#include "mosaic/defines.hpp"
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
    struct Impl;

    Impl* m_impl;

    static RenderSystem* g_instance;

   public:
    RenderSystem(RendererAPIType _apiType);
    virtual ~RenderSystem() override;

    RenderSystem(const RenderSystem&) = delete;
    RenderSystem& operator=(const RenderSystem&) = delete;
    RenderSystem(RenderSystem&&) = default;
    RenderSystem& operator=(RenderSystem&&) = default;

    static std::unique_ptr<RenderSystem> create(RendererAPIType _type);

   public:
    virtual pieces::RefResult<core::System, std::string> initialize() override = 0;
    virtual void shutdown() override = 0;

    pieces::Result<RenderContext*, std::string> createContext(const window::Window* _window);
    void destroyContext(const window::Window* _window);

    inline void destroyAllContexts();

    inline pieces::RefResult<core::System, std::string> update() override;

    inline RenderContext* getContext(const window::Window* _window) const;

    [[nodiscard]] static inline RenderSystem* getInstance()
    {
        if (!g_instance) MOSAIC_ERROR("RenderSystem has not been created yet!");

        return g_instance;
    }
};

} // namespace graphics
} // namespace mosaic
