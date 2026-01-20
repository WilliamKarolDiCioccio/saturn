#include "glfw_window.hpp"

#include <iostream>
#include <stdexcept>

#include <GLFW/glfw3.h>
#include <stb_image.h>
#include <stb_image_resize2.h>

#include "saturn/tools/logger.hpp"

namespace saturn
{
namespace platform
{
namespace glfw
{

pieces::RefResult<window::Window, std::string> GLFWWindow::initialize(
    const window::WindowProperties& _properties)
{
    auto& props = getWindowPropertiesInternal();

    props = _properties;

#ifdef SATURN_PLATFORM_EMSCRIPTEN
    static int windowCount = 0;
    if (windowCount > 0)
    {
        throw std::runtime_error("Only one GLFW window is allowed in Emscripten builds.");
    }
    windowCount++;
#endif

    // Base window hints
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, props.isResizeable ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_VISIBLE, props.isMinimized ? GLFW_FALSE : GLFW_TRUE);
    glfwWindowHint(GLFW_FOCUSED, GLFW_TRUE);
    glfwWindowHint(GLFW_DECORATED, !props.isFullscreen
                                       ? GLFW_TRUE
                                       : GLFW_FALSE); // Remove decoration for fullscreen

#ifndef SATURN_PLATFORM_EMSCRIPTEN
    // Set window position hints if supported
    glfwWindowHint(GLFW_POSITION_X, props.position.x);
    glfwWindowHint(GLFW_POSITION_Y, props.position.y);
#endif

    GLFWmonitor* monitor = nullptr;
    const GLFWvidmode* mode = nullptr;

    // Setup fullscreen if requested
    if (props.isFullscreen)
    {
        monitor = glfwGetPrimaryMonitor();
        mode = glfwGetVideoMode(monitor);
        if (mode)
        {
            glfwWindowHint(GLFW_RED_BITS, mode->redBits);
            glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
            glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
            glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
        }
    }

    const int width = props.size.x;
    const int height = props.size.y;

    m_glfwHandle = glfwCreateWindow(width, height, props.title.c_str(), monitor, nullptr);

    if (!m_glfwHandle)
    {
        throw std::runtime_error("Failed to create GLFW window.");
    }

#ifndef SATURN_PLATFORM_EMSCRIPTEN
    // Move window if not in fullscreen (fullscreen ignores position)
    if (!props.isFullscreen)
    {
        glfwSetWindowPos(m_glfwHandle, props.position.x, props.position.y);
    }

    // Maximize if requested
    if (props.isMaximized)
    {
        glfwMaximizeWindow(m_glfwHandle);
    }
#endif

    registerCallbacks();

    SATURN_DEBUG("Window created: {0} ({1} x {2})", props.title, props.size.x, props.size.y);

    return pieces::OkRef<window::Window, std::string>(*this);
}

void GLFWWindow::shutdown()
{
    if (!m_glfwHandle)
    {
        SATURN_ERROR("Window already destroyed.");
        return;
    }

    unregisterCallbacks();

    glfwDestroyWindow(m_glfwHandle);
    m_glfwHandle = nullptr;

    SATURN_DEBUG("Window destroyed.");
}

void* GLFWWindow::getNativeHandle() const { return static_cast<void*>(m_glfwHandle); }

bool GLFWWindow::shouldClose() const { return glfwWindowShouldClose(m_glfwHandle); }

glm::ivec2 GLFWWindow::getFramebufferSize() const
{
    int width, height;
    glfwGetFramebufferSize(m_glfwHandle, &width, &height);
    return glm::ivec2(width, height);
}

void GLFWWindow::setTitle(const std::string& _title)
{
    glfwSetWindowTitle(m_glfwHandle, _title.c_str());
    getWindowPropertiesInternal().title = _title;
}

void GLFWWindow::setSize(glm::vec2 _size)
{
    glfwSetWindowSize(m_glfwHandle, static_cast<int>(_size.x), static_cast<int>(_size.y));
    getWindowPropertiesInternal().size = _size;
}

void GLFWWindow::setMinimized(bool _minimized)
{
    _minimized ? glfwIconifyWindow(m_glfwHandle) : glfwRestoreWindow(m_glfwHandle);
    getWindowPropertiesInternal().isMinimized = _minimized;
}

void GLFWWindow::setMaximized(bool _maximized)
{
    _maximized ? glfwMaximizeWindow(m_glfwHandle) : glfwRestoreWindow(m_glfwHandle);
    getWindowPropertiesInternal().isMaximized = _maximized;
}

void GLFWWindow::setFullscreen(bool _fullscreen)
{
    if (_fullscreen)
    {
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        glfwSetWindowMonitor(m_glfwHandle, monitor, 0, 0, mode->width, mode->height,
                             mode->refreshRate);
    }
    else
    {
        glfwSetWindowMonitor(m_glfwHandle, nullptr, 100, 100, 800, 600, 0);
    }

    getWindowPropertiesInternal().isFullscreen = _fullscreen;
}

void GLFWWindow::setResizeable(bool _resizeable)
{
    glfwSetWindowAttrib(m_glfwHandle, GLFW_RESIZABLE, _resizeable ? GLFW_TRUE : GLFW_FALSE);
    getWindowPropertiesInternal().isResizeable = _resizeable;
}

void GLFWWindow::setVSync(bool enabled)
{
    enabled ? glfwSwapInterval(1) : glfwSwapInterval(0);
    getWindowPropertiesInternal().isVSync = enabled;
}

void GLFWWindow::setWindowIcon(const std::string& _path, int _width, int _height)
{
    int w, h, channels;
    unsigned char* data = stbi_load(_path.c_str(), &w, &h, &channels, 4);

    if (!data)
    {
        SATURN_ERROR("Failed to load window icon: {0}", _path.c_str());
        return;
    }

    if (_width > 0 && _height > 0 && (_width != w || _height != h))
    {
        unsigned char* resized = (unsigned char*)malloc(_width * _height * 4);

        stbir_resize_uint8_srgb(data, w, h, 0, resized, _width, _height, 0,
                                static_cast<stbir_pixel_layout>(4));

        if (!resized)
        {
            SATURN_ERROR("Failed to resize window icon: {0}", _path.c_str());
            stbi_image_free(data);
            return;
        }

        stbi_image_free(data);

        data = resized;
        w = _width;
        h = _height;
    }

    GLFWimage image;
    image.width = w;
    image.height = h;
    image.pixels = data;

    glfwSetWindowIcon(m_glfwHandle, 1, &image);
    stbi_image_free(data);
}

void GLFWWindow::resetWindowIcon() { glfwSetWindowIcon(m_glfwHandle, 0, nullptr); }

void GLFWWindow::setCursorMode(window::CursorMode _mode)
{
    switch (_mode)
    {
        case window::CursorMode::normal:
            glfwSetInputMode(m_glfwHandle, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            break;
        case window::CursorMode::captured:
#ifndef SATURN_PLATFORM_EMSCRIPTEN
            glfwSetInputMode(m_glfwHandle, GLFW_CURSOR, GLFW_CURSOR_CAPTURED);
#else
            SATURN_ERROR("GLFW_CURSOR_CAPTURED is not supported in Emscripten builds.");
#endif
            break;
        case window::CursorMode::hidden:
            glfwSetInputMode(m_glfwHandle, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
            break;
        case window::CursorMode::disabled:
            glfwSetInputMode(m_glfwHandle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            break;
    }
}

void GLFWWindow::setCursorType(window::CursorType _type)
{
    auto& props = getCursorPropertiesInternal();

    if (props.currentType == _type) return;

    props.currentType = _type;
    setCursorIcon(props.srcPaths[static_cast<int>(_type)]);
}

void GLFWWindow::setCursorTypeIcon(window::CursorType _type, const std::string& _path, int _width,
                                   int _height)
{
    auto& props = getCursorPropertiesInternal();

    if (props.srcPaths[static_cast<int>(_type)] == _path) return;

    props.srcPaths[static_cast<int>(_type)] = _path;
    setCursorIcon(_path, _width, _height);
}

void GLFWWindow::setCursorIcon(const std::string& _path, int _width, int _height)
{
    int w, h, channels;
    unsigned char* data = stbi_load(_path.c_str(), &w, &h, &channels, 4);

    if (!data)
    {
        SATURN_ERROR("Failed to load cursor icon: {0}", _path.c_str());
        return;
    }

    if (_width > 0 && _height > 0 && (_width != w || _height != h))
    {
        unsigned char* resized = (unsigned char*)malloc(_width * _height * 4);

        stbir_resize_uint8_linear(data, w, h, 0, resized, _width, _height, 0,
                                  static_cast<stbir_pixel_layout>(4));

        if (!resized)
        {
            SATURN_ERROR("Failed to resize cursor icon: {0}", _path.c_str());
            stbi_image_free(data);
            return;
        }

        stbi_image_free(data);

        data = resized;
        w = _width;
        h = _height;
    }

    GLFWimage image;
    image.width = w;
    image.height = h;
    image.pixels = data;

    GLFWcursor* cursor = glfwCreateCursor(&image, w / 2, h / 2);

    if (!cursor)
    {
        SATURN_ERROR("Failed to create cursor from image: {0}", _path.c_str());
        stbi_image_free(data);
        return;
    }

    glfwSetCursor(m_glfwHandle, cursor);
    stbi_image_free(data);
}

void GLFWWindow::resetCursorIcon() { glfwSetCursor(m_glfwHandle, nullptr); }

void GLFWWindow::setClipboardString(const std::string& _string)
{
    glfwSetClipboardString(m_glfwHandle, _string.c_str());
}

std::string GLFWWindow::getClipboardString() const
{
    const char* clipboardString = glfwGetClipboardString(m_glfwHandle);

    if (!clipboardString) return std::string();

    return std::string(clipboardString);
}

void GLFWWindow::registerCallbacks()
{
    glfwSetWindowUserPointer(m_glfwHandle, this);

    glfwSetWindowCloseCallback(m_glfwHandle, windowCloseCallback);
    glfwSetWindowFocusCallback(m_glfwHandle, windowFocusCallback);
    glfwSetWindowSizeCallback(m_glfwHandle, windowSizeCallback);
    glfwSetWindowRefreshCallback(m_glfwHandle, windowRefreshCallback);
    glfwSetWindowIconifyCallback(m_glfwHandle, windowIconifyCallback);
    glfwSetWindowMaximizeCallback(m_glfwHandle, windowMaximizeCallback);
    glfwSetDropCallback(m_glfwHandle, windowDropCallback);
    glfwSetScrollCallback(m_glfwHandle, windowScrollCallback);
    glfwSetCursorEnterCallback(m_glfwHandle, windowCursorEnterCallback);
    glfwSetWindowPosCallback(m_glfwHandle, windowPosCallback);
    glfwSetWindowContentScaleCallback(m_glfwHandle, windowContentScaleCallback);
    glfwSetCharCallback(m_glfwHandle, windowCharCallback);
}

void GLFWWindow::unregisterCallbacks()
{
    glfwSetWindowCloseCallback(m_glfwHandle, nullptr);
    glfwSetWindowFocusCallback(m_glfwHandle, nullptr);
    glfwSetWindowSizeCallback(m_glfwHandle, nullptr);
    glfwSetWindowRefreshCallback(m_glfwHandle, nullptr);
    glfwSetWindowIconifyCallback(m_glfwHandle, nullptr);
    glfwSetWindowMaximizeCallback(m_glfwHandle, nullptr);
    glfwSetDropCallback(m_glfwHandle, nullptr);
    glfwSetScrollCallback(m_glfwHandle, nullptr);
    glfwSetCursorEnterCallback(m_glfwHandle, nullptr);
    glfwSetWindowPosCallback(m_glfwHandle, nullptr);
    glfwSetWindowContentScaleCallback(m_glfwHandle, nullptr);
    glfwSetCharCallback(m_glfwHandle, nullptr);

    glfwSetWindowUserPointer(m_glfwHandle, nullptr);
}

void GLFWWindow::windowCloseCallback(GLFWwindow* window)
{
    GLFWWindow* instance = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));

    if (instance)
    {
        instance->invokeWindowCloseCallbacks();
    }
    else
    {
        SATURN_ERROR("GLFWWindow::windowCloseCallback(): callback receiver is null.");
    }
}

void GLFWWindow::windowFocusCallback(GLFWwindow* window, int focused)
{
    GLFWWindow* instance = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));

    if (instance)
    {
        instance->invokeWindowFocusCallbacks(focused);
    }
    else
    {
        SATURN_ERROR("GLFWWindow::windowFocusCallback(): callback receiver is null.");
    }
}

void GLFWWindow::windowSizeCallback(GLFWwindow* window, int width, int height)
{
    GLFWWindow* instance = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));

    if (instance)
    {
        instance->getWindowPropertiesInternal().size = glm::ivec2(width, height);
        instance->invokeWindowResizeCallbacks(width, height);
    }
    else
    {
        SATURN_ERROR("GLFWWindow::windowSizeCallback(): callback receiver is null.");
    }
}

void GLFWWindow::windowRefreshCallback(GLFWwindow* window)
{
    GLFWWindow* instance = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));

    if (instance)
    {
        instance->invokeWindowRefreshCallbacks();
    }
    else
    {
        SATURN_ERROR("GLFWWindow::windowRefreshCallback(): callback receiver is null.");
    }
}

void GLFWWindow::windowIconifyCallback(GLFWwindow* window, int iconified)
{
    GLFWWindow* instance = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));

    if (instance)
    {
        instance->getWindowPropertiesInternal().isMinimized = (iconified == GLFW_TRUE);
        instance->invokeWindowIconifyCallbacks(iconified);
    }
    else
    {
        SATURN_ERROR("GLFWWindow::windowIconifyCallback(): callback receiver is null.");
    }
}

void GLFWWindow::windowMaximizeCallback(GLFWwindow* window, int maximized)
{
    GLFWWindow* instance = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));

    if (instance)
    {
        instance->getWindowPropertiesInternal().isMaximized = (maximized == GLFW_TRUE);
        instance->invokeWindowMaximizeCallbacks(maximized);
    }
    else
    {
        SATURN_ERROR("GLFWWindow::windowMaximizeCallback(): callback receiver is null.");
    }
}

void GLFWWindow::windowDropCallback(GLFWwindow* window, int count, const char** paths)
{
    GLFWWindow* instance = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));

    if (instance)
    {
        instance->invokeWindowDropCallbacks(count, paths);
    }
    else
    {
        SATURN_ERROR("GLFWWindow::windowDropCallback(): callback receiver is null.");
    }
}

void GLFWWindow::windowScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    GLFWWindow* instance = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));

    if (instance)
    {
        instance->invokeWindowScrollCallbacks(xoffset, yoffset);
    }
    else
    {
        SATURN_ERROR("GLFWWindow::windowScrollCallback(): callback receiver is null.");
    }
}

void GLFWWindow::windowCursorEnterCallback(GLFWwindow* window, int entered)
{
    GLFWWindow* instance = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));

    if (instance)
    {
        instance->invokeWindowCursorEnterCallbacks(entered);
    }
    else
    {
        SATURN_ERROR("GLFWWindow::windowCursorEnterCallback(): callback receiver is null.");
    }
}

void GLFWWindow::windowPosCallback(GLFWwindow* window, int x, int y)
{
    GLFWWindow* instance = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));

    if (instance)
    {
        instance->getWindowPropertiesInternal().position = glm::ivec2(x, y);
        instance->invokeWindowPosCallbacks(x, y);
    }
    else
    {
        SATURN_ERROR("GLFWWindow::windowPosCallback(): callback receiver is null.");
    }
}

void GLFWWindow::windowContentScaleCallback(GLFWwindow* window, float xscale, float yscale)
{
    GLFWWindow* instance = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));

    if (instance)
    {
        instance->invokeWindowContentScaleCallbacks(xscale, yscale);
    }
    else
    {
        SATURN_ERROR("GLFWWindow::windowContentScaleCallback(): callback receiver is null.");
    }
}

void GLFWWindow::windowCharCallback(GLFWwindow* _window, unsigned int _codepoint)
{
    GLFWWindow* instance = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(_window));

    if (instance)
    {
        instance->invokeWindowCharCallbacks(_codepoint);
    }
    else
    {
        SATURN_ERROR("GLFWWindow::windowCharCallback(): callback receiver is null.");
    }
}

} // namespace glfw
} // namespace platform
} // namespace saturn
