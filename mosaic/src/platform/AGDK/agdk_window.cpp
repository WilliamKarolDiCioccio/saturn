#include "agdk_window.hpp"

namespace saturn
{
namespace platform
{
namespace agdk
{

pieces::RefResult<window::Window, std::string> AGDKWindow::initialize(
    const window::WindowProperties& _properties)
{
    static int windowCount = 0;

    if (windowCount > 0)
    {
        throw std::runtime_error("Only one GLFW window is allowed in Emscripten builds.");
    }

    windowCount++;

    return pieces::OkRef<window::Window, std::string>(*this);
}

void AGDKWindow::shutdown() {}

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

void AGDKWindow::setTitle([[maybe_unused]] const std::string& _title)
{
    // AGDK does not provide a method to set the window title.
}

void AGDKWindow::setMinimized([[maybe_unused]] bool _minimized)
{
    // AGDK does not provide a method to set the window to minimized.
}

void AGDKWindow::setMaximized([[maybe_unused]] bool _maximized)
{
    // AGDK does not provide a method to set the window to maximized.
}

void AGDKWindow::setFullscreen([[maybe_unused]] bool _fullscreen)
{
    // AGDK does not provide a method to set the window to fullscreen.
}

void AGDKWindow::setSize([[maybe_unused]] glm::vec2 _size)
{
    // AGDK does not provide a method to set the window size directly.
}

void AGDKWindow::setResizeable([[maybe_unused]] bool _resizeable)
{
    // AGDK does not provide a method to set the window to be resizable.
}

void AGDKWindow::setVSync([[maybe_unused]] bool _enabled)
{
    SATURN_WARN("AGDKWindow::setVSync: Not implemented for AGDK platform.");
}

void AGDKWindow::setCursorMode([[maybe_unused]] window::CursorMode _mode)
{
    // AGDK does not provide a method to set the cursor mode.
}

void AGDKWindow::setCursorType([[maybe_unused]] window::CursorType _type)
{
    // AGDK does not provide a method to set the cursor type.
}

void AGDKWindow::setCursorTypeIcon([[maybe_unused]] window::CursorType _type,
                                   [[maybe_unused]] const std::string& _path,
                                   [[maybe_unused]] int _width, [[maybe_unused]] int _height)
{
    // AGDK does not provide a method to set the cursor type icon.
}

void AGDKWindow::setCursorIcon([[maybe_unused]] const std::string& _path,
                               [[maybe_unused]] int _width, [[maybe_unused]] int _height)
{
    // AGDK does not provide a method to set the cursor icon.
}

void AGDKWindow::resetCursorIcon()
{
    // AGDK does not provide a method to reset the cursor icon.
}

void AGDKWindow::setWindowIcon([[maybe_unused]] const std::string& _path,
                               [[maybe_unused]] int _width, [[maybe_unused]] int _height)
{
    // AGDK does not provide a method to set the window icon.
}

void AGDKWindow::resetWindowIcon()
{
    // AGDK does not provide a method to reset the window icon.
}

void AGDKWindow::setClipboardString([[maybe_unused]] const std::string& _string)
{
    auto* helper = JNIHelper::getInstance();

    JNIEnv* env = helper->getEnv();
    if (!env)
    {
        SATURN_ERROR("AGDKWindow::setClipboardString: JNI environment is not available.");
        return;
    }

    jstring jContent = helper->stringToJstring(_string);

    // See SystemServices.setClipboardString(String content)
    helper->callStaticVoidMethod("com/saturn/engine_bridge/SystemServices", "setClipboard",
                                 "(Ljava/lang/String;)V", jContent);

    if (jContent) env->DeleteLocalRef(jContent);
}

std::string AGDKWindow::getClipboardString() const
{
    auto* helper = JNIHelper::getInstance();

    JNIEnv* env = helper->getEnv();
    if (!env)
    {
        SATURN_ERROR("AGDKWindow::getClipboardString: JNI environment is not available.");
        return std::string();
    }

    jstring jContent = helper->callStaticMethod<jstring>("com/saturn/engine_bridge/SystemServices",
                                                         "getClipboard", "()Ljava/lang/String;");

    if (!jContent)
    {
        SATURN_ERROR("AGDKWindow::getClipboardString: Failed to get clipboard content.");
        return std::string();
    }

    std::string content = helper->jstringToString(jContent);

    return content;
}

} // namespace agdk
} // namespace platform
} // namespace saturn
