#pragma once

#include "mosaic/input/sources/text_input_source.hpp"

#include <GLFW/glfw3.h>

namespace mosaic
{
namespace platform
{
namespace glfw
{

class GLFWTextInputSource : public input::TextInputSource
{
   private:
    GLFWwindow* m_nativeHandle;
    size_t m_charCallbackId, m_focusCallbackId;
    std::vector<char32_t> m_codepointsBuffer;

   public:
    GLFWTextInputSource(window::Window* window);
    ~GLFWTextInputSource() override = default;

   public:
    pieces::RefResult<input::InputSource, std::string> initialize() override;
    void shutdown() override;
    void pollDevice() override;

   private:
    [[nodiscard]] std::vector<char32_t> queryCodepoints() override;
};

} // namespace glfw
} // namespace platform
} // namespace mosaic
