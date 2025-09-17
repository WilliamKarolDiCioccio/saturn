#pragma once

#include <string>
#include <fstream>
#include <unordered_map>
#include <functional>
#include <nlohmann/json.hpp>

#include <pieces/core/result.hpp>

#include "mosaic/core/system.hpp"

#include "input_context.hpp"

namespace mosaic
{
namespace input
{

/**
 * @brief The `InputSystem` class is responsible for managing input contexts for different windows.
 *
 * `InputSystem`'s responsibilities include:
 *
 * - Registering and unregistering input contexts for each window.
 *
 * - Managing GLFW window events loop.
 *
 * The input system as a whole consists of multiple componenets, ensuring clean separation of
 * concerns. Each of those components adds an abstraction layer on top of the previous one, allowing
 * for easy extensibility and maintainability.
 *
 * @note This class is a singleton and should be accessed through the static method
 * `getGlobalInputSystem()`. It is not meant to be instantiated directly.
 *
 * @see InputContext
 * @see InputSource
 * @see Window
 * @see https://www.glfw.org/documentation.html
 */
class MOSAIC_API InputSystem final : public core::EngineSystem
{
   public:
    static InputSystem* g_instance;

    std::unordered_map<const window::Window*, std::unique_ptr<InputContext>> m_contexts;

   public:
    InputSystem() : EngineSystem(core::EngineSystemType::input)
    {
        assert(!g_instance && "InputSystem already exists!");
        g_instance = this;
    };

    ~InputSystem() override { g_instance = nullptr; }

    InputSystem(const InputSystem&) = delete;
    InputSystem& operator=(const InputSystem&) = delete;
    InputSystem(InputSystem&&) = default;
    InputSystem& operator=(InputSystem&&) = default;

   public:
    pieces::RefResult<System, std::string> initialize() override;
    void shutdown();

    pieces::Result<InputContext*, std::string> registerWindow(window::Window* _window);
    void unregisterWindow(window::Window* _window);

    inline void unregisterAllWindows()
    {
        for (auto& [window, context] : m_contexts) context->shutdown();

        m_contexts.clear();
    }

    /**
     * @brief Updates all registered input contexts.
     *
     * This should be invocated after window system update in the main loop of the application.
     */
    inline pieces::RefResult<System, std::string> update() override
    {
        for (auto& [window, context] : m_contexts) context->update();

        return pieces::OkRef<System, std::string>(*this);
    }

    inline InputContext* getContext(const window::Window* _window) const
    {
        if (m_contexts.find(_window) != m_contexts.end()) return m_contexts.at(_window).get();

        return nullptr;
    }

    [[nodiscard]] static InputSystem* getGlobalInputSystem()
    {
        if (!g_instance) MOSAIC_ERROR("InputSystem has not been created yet!");

        return g_instance;
    }
};

} // namespace input
} // namespace mosaic
