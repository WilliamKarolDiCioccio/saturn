#pragma once

#include "mosaic/input/sources/mouse_input_source.hpp"

#include <GLFW/glfw3.h>

namespace mosaic
{
namespace platform
{
namespace glfw
{

class GLFWMouseInputSource : public input::MouseInputSource
{
   private:
    GLFWwindow* m_nativeHandle;
    glm::vec2 m_cumulativeWheelOffse;

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
    [[nodiscard]] glm::vec2 queryWheelOffset() const override;
};

} // namespace glfw
} // namespace platform
} // namespace mosaic
