#pragma once

#include "saturn/input/sources/mouse_input_source.hpp"

#include <GLFW/glfw3.h>

namespace saturn
{
namespace platform
{
namespace glfw
{

class GLFWMouseInputSource : public input::MouseInputSource
{
   private:
    GLFWwindow* m_nativeHandle;
    size_t m_focusCallbackId, m_scrollCallbackId;
    glm::vec2 m_wheelOffsetBuffer;

   public:
    GLFWMouseInputSource(window::Window* _window);
    ~GLFWMouseInputSource() override = default;

   public:
    pieces::RefResult<input::InputSource, std::string> initialize() override;
    void shutdown() override;

    void pollDevice() override;

   private:
    [[nodiscard]] input::InputAction queryButtonState(input::MouseButton _button) const override;
    [[nodiscard]] glm::vec2 queryCursorPosition() const override;
    [[nodiscard]] glm::vec2 queryWheelOffset() override;
};

} // namespace glfw
} // namespace platform
} // namespace saturn
