#pragma once

#include <string>

#include <pieces/core/result.hpp>

#include "saturn/core/system.hpp"

#include "input_context.hpp"

namespace saturn
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
class SATURN_API InputSystem final : public core::EngineSystem
{
   public:
    static InputSystem* g_instance;

    struct Impl;

    Impl* m_impl;

   public:
    InputSystem();
    ~InputSystem() override;

    InputSystem(const InputSystem&) = delete;
    InputSystem& operator=(const InputSystem&) = delete;
    InputSystem(InputSystem&&) = default;
    InputSystem& operator=(InputSystem&&) = default;

   public:
    pieces::RefResult<System, std::string> initialize() override;
    void shutdown();

    pieces::Result<InputContext*, std::string> registerWindow(window::Window* _window);
    void unregisterWindow(window::Window* _window);

    void unregisterAllWindows();

    /**
     * @brief Updates all registered input contexts.
     *
     * This should be invocated after window system update in the main loop of the application.
     */
    pieces::RefResult<System, std::string> update() override;

    InputContext* getContext(const window::Window* _window) const;

    [[nodiscard]] static inline InputSystem* getInstance()
    {
        if (!g_instance) SATURN_ERROR("InputSystem has not been created yet!");

        return g_instance;
    }
};

} // namespace input
} // namespace saturn
