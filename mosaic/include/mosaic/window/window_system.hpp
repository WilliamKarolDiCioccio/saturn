#pragma once

#include <string>
#include <memory>
#include <unordered_map>

#include <pieces/result.hpp>

#include "mosaic/core/logger.hpp"

#include "window.hpp"

namespace mosaic
{
namespace window
{

/**
 * @brief The `WindowSystem` class is responsible for managing windows in the application.
 *
 * `WindowSystem`'s responsibilities include:
 *
 * - Creating and destroying windows.
 *
 * - Managing window properties and states.
 *
 * - Handling window updates and events.
 *
 * @note This class is not meant to be instantiated directly. Use the static method
 * `create()` to obtain an instance.
 */
class MOSAIC_API WindowSystem
{
   public:
    std::unordered_map<std::string, std::unique_ptr<Window>> m_windows;

   public:
    WindowSystem() = default;
    virtual ~WindowSystem() = default;

    WindowSystem(const WindowSystem&) = delete;
    WindowSystem& operator=(const WindowSystem&) = delete;
    WindowSystem(WindowSystem&&) = default;
    WindowSystem& operator=(WindowSystem&&) = default;

    static std::unique_ptr<WindowSystem> create();

   public:
    virtual pieces::RefResult<WindowSystem, std::string> initialize() = 0;
    virtual void shutdown() = 0;

    pieces::Result<Window*, std::string> createWindow(
        const std::string& _windowId, const WindowProperties& _properties = WindowProperties());
    void destroyWindow(const std::string& _windowId);

    inline void destroyAllWindows()
    {
        for (auto& [window, context] : m_windows)
        {
            context->shutdown();
        }

        m_windows.clear();
    }

    /**
     * @brief Updates all active windows.
     *
     * This is the first invocation in the main loop of the application. It should be called before
     * any other to ensure fresh input data is available and minimize latency.
     *
     * On most platforms input handling is tied to the window system.
     */
    virtual void update() const = 0;

    [[nodiscard]] inline Window* getWindow(const std::string& _windowId) const
    {
        if (m_windows.find(_windowId) != m_windows.end())
        {
            return m_windows.at(_windowId).get();
        }

        return nullptr;
    }
};

} // namespace window
} // namespace mosaic
