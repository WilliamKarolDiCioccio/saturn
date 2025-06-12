#pragma once

#include <string>
#include <vector>
#include <queue>
#include <optional>
#include <functional>

#include <pieces/result.hpp>

#include "mosaic/core/logger.hpp"
#include "mosaic/core/window.hpp"

#include "input_mappings.hpp"

namespace mosaic
{
namespace input
{

/**
 * @brief Abstract base class for raw input handling.
 *
 * This class provides a generic interface for input handling that can be
 * implemented by different input backends (GLFW, SDL, etc.).
 * It defines the common input data types and methods that all implementations should provide.
 */
class RawInputHandler
{
   public:
    using KeyboardKeyInputData = InputAction;
    using MouseButtonInputData = InputAction;
    using MouseScrollInputData = glm::vec2;
    using CursorPosInputData = glm::vec2;

   protected:
    void* m_nativeHandle;
    bool m_isActive;
    std::queue<MouseScrollInputData> m_mouseScrollQueue;

   public:
    RawInputHandler(const core::Window* _window);
    virtual ~RawInputHandler() = default;

    RawInputHandler(RawInputHandler&) = delete;
    RawInputHandler& operator=(RawInputHandler&) = delete;
    RawInputHandler(const RawInputHandler&) = delete;
    RawInputHandler& operator=(const RawInputHandler&) = delete;

    static std::unique_ptr<RawInputHandler> create(core::Window* _window);

   public:
    virtual pieces::RefResult<RawInputHandler, std::string> initialize() = 0;
    virtual void shutdown() = 0;

    inline bool isActive() const { return m_isActive; }

    // Input handling methods
    virtual KeyboardKeyInputData getKeyboardKeyInput(KeyboardKey key) const = 0;
    virtual MouseButtonInputData getMouseButtonInput(MouseButton button) const = 0;
    virtual CursorPosInputData getCursorPosInput() = 0;

    // Input state checking methods

    inline bool mouseScrollInputAvailable() const { return !m_mouseScrollQueue.empty(); }

    inline MouseScrollInputData getMouseScrollInput()
    {
        if (m_mouseScrollQueue.empty())
        {
            return MouseScrollInputData(0.0f, 0.0f);
        }

        MouseScrollInputData scrollInput = m_mouseScrollQueue.front();
        m_mouseScrollQueue.pop();
        return scrollInput;
    }

   protected:
    inline void addMouseScrollInput(double xoffset, double yoffset)
    {
        m_mouseScrollQueue.push(MouseScrollInputData(xoffset, yoffset));
    }

    inline void setActive(bool active) { m_isActive = active; }
};

} // namespace input
} // namespace mosaic
