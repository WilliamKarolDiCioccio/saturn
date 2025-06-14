#pragma once

#include "mosaic/input/raw_input_handler.hpp"

#include <GLFW/glfw3.h>

namespace mosaic
{
namespace platform
{
namespace glfw
{

/**
 * @brief GLFW-specific implementation of the RawInputHandler class.
 *
 * This class provides concrete implementation of input handling using GLFW.
 * Due to latency issues, it uses polling for keyboard and mouse button input,
 * but still uses callbacks for mouse scroll input since GLFW doesn't provide
 * a way to poll scroll input.
 *
 * @note Callbacks showcase high latency issues, that's why this class uses polling for keyboard and
 * mouse button input.
 */
class GLFWRawInputHandler : public input::RawInputHandler
{
   private:
    window::Window* m_window;

   public:
    GLFWRawInputHandler(const window::Window* window);
    ~GLFWRawInputHandler() override = default;

   public:
    pieces::RefResult<input::RawInputHandler, std::string> initialize() override;
    void shutdown() override;

    input::RawInputHandler::KeyboardKeyInputData getKeyboardKeyInput(
        input::KeyboardKey key) const override;
    input::RawInputHandler::MouseButtonInputData getMouseButtonInput(
        input::MouseButton button) const override;
    input::RawInputHandler::CursorPosInputData getCursorPosInput() override;

   private:
    void registerCallbacks();
};

} // namespace glfw
} // namespace platform
} // namespace mosaic
