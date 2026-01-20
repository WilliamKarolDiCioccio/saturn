#pragma once

#include <string>
#include <memory>
#include <unordered_map>

#include <pieces/core/result.hpp>

#include "saturn/tools/logger.hpp"
#include "saturn/core/system.hpp"

#include "window.hpp"

namespace saturn
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
class SATURN_API WindowSystem : public core::EngineSystem
{
   private:
    static WindowSystem* g_instance;

    struct Impl;

    Impl* m_impl;

   public:
    WindowSystem();
    virtual ~WindowSystem() override;

    WindowSystem(const WindowSystem&) = delete;
    WindowSystem& operator=(const WindowSystem&) = delete;
    WindowSystem(WindowSystem&&) = default;
    WindowSystem& operator=(WindowSystem&&) = default;

    static std::unique_ptr<WindowSystem> create();

   public:
    virtual pieces::RefResult<System, std::string> initialize() override = 0;
    virtual void shutdown() override = 0;

    pieces::Result<Window*, std::string> createWindow(
        const std::string& _windowId, const WindowProperties& _properties = WindowProperties());
    void destroyWindow(const std::string& _windowId);

    void destroyAllWindows();

    /**
     * @brief Updates all active windows.
     *
     * This is the first invocation in the main loop of the application. It should be called before
     * any other to ensure fresh input data is available and minimize latency.
     *
     * On most platforms input handling is tied to the window system.
     */
    virtual pieces::RefResult<System, std::string> update() override = 0;

    [[nodiscard]] Window* getWindow(const std::string& _windowId) const;

    [[nodiscard]] inline size_t getWindowCount() const;

    [[nodiscard]] static inline WindowSystem* getInstance()
    {
        if (!g_instance) SATURN_ERROR("WindowSystem has not been created yet!");

        return g_instance;
    }
};

} // namespace window
} // namespace saturn
