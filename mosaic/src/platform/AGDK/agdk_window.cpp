#include "agdk_window.hpp"

namespace mosaic
{
namespace platform
{
namespace agdk
{

AGDKWindow::AGDKWindow(const std::string& _title, glm::ivec2 _size) : core::Window(_title, _size)
{
    static int windowCount = 0;

    if (windowCount > 0)
    {
        throw std::runtime_error("Only one GLFW window is allowed in Emscripten builds.");
    }

    windowCount++;
}

void* AGDKWindow::getNativeHandle() const
{
    auto context =
        static_cast<AGDKPlatformContext*>(AGDKPlatform::getInstance()->getPlatformContext());
    auto window = context->getCurrentWindow();

    return static_cast<void*>(window);
}

bool AGDKWindow::shouldClose() const
{
    // AGDK does not have a direct method to check if the window should close.
    // This is usually handled by the application logic or event loop.
    return false;
}

glm::ivec2 AGDKWindow::getFramebufferSize() const
{
    auto context =
        static_cast<AGDKPlatformContext*>(AGDKPlatform::getInstance()->getPlatformContext());
    auto window = context->getCurrentWindow();

    int x = ANativeWindow_getWidth(window);
    int y = ANativeWindow_getHeight(window);

    return glm::ivec2(x, y);
}

void AGDKWindow::setTitle(const std::string& _title)
{
    // AGDK does not provide a method to set the window title.
}

void AGDKWindow::setMinimized(bool _minimized)
{
    // AGDK does not provide a method to set the window to minimized.
}

void AGDKWindow::setMaximized(bool _maximized)
{
    // AGDK does not provide a method to set the window to maximized.
}

void AGDKWindow::setFullscreen(bool _fullscreen)
{
    // AGDK does not provide a method to set the window to fullscreen.
}

void AGDKWindow::setSize(glm::vec2 _size)
{
    // AGDK does not provide a method to set the window size directly.
}

void AGDKWindow::setResizeable(bool _resizeable)
{
    // AGDK does not provide a method to set the window to be resizable.
}

void AGDKWindow::setVSync(bool _enabled)
{
    MOSAIC_WARN("AGDKWindow::setVSync: Not implemented for AGDK platform.");
}

void AGDKWindow::setCursorMode(core::CursorMode _mode)
{
    // AGDK does not provide a method to set the cursor mode.
}

void AGDKWindow::setCursorType(core::CursorType _type)
{
    // AGDK does not provide a method to set the cursor type.
}

void AGDKWindow::setCursorTypeIcon(core::CursorType _type, const std::string& _path, int _width,
                                   int _height)
{
    // AGDK does not provide a method to set the cursor type icon.
}

void AGDKWindow::setCursorIcon(const std::string& _path, int _width, int _height)
{
    // AGDK does not provide a method to set the cursor icon.
}

void AGDKWindow::resetCursorIcon()
{
    // AGDK does not provide a method to reset the cursor icon.
}

void AGDKWindow::setWindowIcon(const std::string& _path, int _width, int _height)
{
    // AGDK does not provide a method to set the window icon.
}

void AGDKWindow::resetWindowIcon()
{
    // AGDK does not provide a method to reset the window icon.
}

void AGDKWindow::setClipboardString(const std::string& _string)
{
    MOSAIC_WARN(
        "AGDKWindow::setClipboardString: Not implemented for AGDK platform. (requires JNI bridge)");
}

std::string AGDKWindow::getClipboardString() const
{
    MOSAIC_WARN(
        "AGDKWindow::getClipboardString: Not implemented for AGDK platform. (requires JNI bridge)");
    return "";
}

} // namespace agdk
} // namespace platform
} // namespace mosaic
