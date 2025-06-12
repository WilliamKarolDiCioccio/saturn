#pragma once

#include <string>
#include <fstream>
#include <unordered_map>
#include <functional>
#include <nlohmann/json.hpp>

#include <pieces/result.hpp>

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
 * @see InputArena
 * @see RawInputHandler
 * @see Window
 * @see https://www.glfw.org/documentation.html
 */
class MOSAIC_API InputSystem
{
   public:
    std::unordered_map<const core::Window*, std::unique_ptr<InputContext>> m_contexts;

   public:
    InputSystem() = default;

    InputSystem(const InputSystem&) = delete;
    InputSystem& operator=(const InputSystem&) = delete;
    InputSystem(InputSystem&&) = default;
    InputSystem& operator=(InputSystem&&) = default;

   public:
    pieces::RefResult<InputSystem, std::string> initialize();
    void shutdown();

    pieces::Result<InputContext*, std::string> registerWindow(core::Window* _window);
    void unregisterWindow(core::Window* _window);

    inline void unregisterAllWindows()
    {
        for (auto& [window, context] : m_contexts)
        {
            context->shutdown();
        }

        m_contexts.clear();
    }

    /**
     * @brief Updates all registered input contexts.
     *
     * This is the first invocation in the main loop of the application. It should be called before
     * any other input handling functions to ensure fresh input data is available and minimize
     * latency.
     */
    inline void poll() const
    {
        for (auto& [window, context] : m_contexts)
        {
            context->update();
        }
    }

    inline InputContext* getContext(const core::Window* _window) const
    {
        if (m_contexts.find(_window) != m_contexts.end())
        {
            return m_contexts.at(_window).get();
        }

        return nullptr;
    }
};

} // namespace input
} // namespace mosaic
