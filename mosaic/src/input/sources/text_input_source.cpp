#include "mosaic/input/sources/text_input_source.hpp"

#if defined(MOSAIC_PLATFORM_DESKTOP) || defined(MOSAIC_PLATFORM_WEB)
#include "platform/GLFW/glfw_text_input_source.hpp"
#endif

#include "mosaic/utils/unicode.hpp"

using namespace std::chrono_literals;

namespace mosaic
{
namespace input
{

TextInputSource::TextInputSource(window::Window* _window) : InputSource(_window)
{
    auto currentTime = std::chrono::high_resolution_clock::now();

    m_textEvents.push(TextInputEvent({}, "", currentTime, m_pollCount));
};

std::unique_ptr<TextInputSource> TextInputSource::create(window::Window* _window)
{
#if defined(MOSAIC_PLATFORM_DESKTOP) || defined(MOSAIC_PLATFORM_WEB)
    return std::make_unique<platform::glfw::GLFWTextInputSource>(_window);
#else
    throw std::runtime_error("Mouse input source not supported on mobile devices");
#endif
}

void TextInputSource::processInput()
{
    auto currentTime = std::chrono::high_resolution_clock::now();

    if (!isActive()) return;

    pollDevice();

    auto codepoints = queryCodepoints();

    if (codepoints.empty()) return;

    TextInputEvent event = {
        codepoints,
        utils::CodepointsToUtf8(codepoints),
        currentTime,
        m_pollCount,
    };

    m_textEvents.push(event);
}

} // namespace input
} // namespace mosaic
