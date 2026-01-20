#pragma once

#include "saturn/input/sources/keyboard_input_source.hpp"

#include <GLFW/glfw3.h>

namespace saturn
{
namespace platform
{
namespace glfw
{

class GLFWKeyboardInputSource : public input::KeyboardInputSource
{
   private:
    GLFWwindow* m_nativeHandle;
    size_t m_focusCallbackId;

   public:
    GLFWKeyboardInputSource(window::Window* _window);
    ~GLFWKeyboardInputSource() override = default;

   public:
    pieces::RefResult<input::InputSource, std::string> initialize() override;
    void shutdown() override;

    void pollDevice() override;

   private:
    [[nodiscard]] input::InputAction queryKeyState(input::KeyboardKey _key) const override;
};

} // namespace glfw
} // namespace platform
} // namespace saturn
