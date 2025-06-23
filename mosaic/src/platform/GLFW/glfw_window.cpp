#include "glfw_window.hpp"

#include <iostream>
#include <stdexcept>

#include <GLFW/glfw3.h>
#include <stb_image.h>
#include <stb_image_resize2.h>

#include "mosaic/core/logger.hpp"

namespace mosaic
{
namespace platform
{
namespace glfw
{

pieces::RefResult<window::Window, std::string> GLFWWindow::initialize(
    const window::WindowProperties& _properties)
{
    m_properties = _properties;

#ifdef MOSAIC_PLATFORM_EMSCRIPTEN
    static int windowCount = 0;
    if (windowCount > 0)
    {
        throw std::runtime_error("Only one GLFW window is allowed in Emscripten builds.");
    }
    windowCount++;
#endif

    // Base window hints
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, m_properties.isResizeable ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_VISIBLE, m_properties.isMinimized ? GLFW_FALSE : GLFW_TRUE);
    glfwWindowHint(GLFW_FOCUSED, GLFW_TRUE);
    glfwWindowHint(GLFW_DECORATED, !m_properties.isFullscreen
                                       ? GLFW_TRUE
                                       : GLFW_FALSE); // Remove decoration for fullscreen

#ifndef MOSAIC_PLATFORM_EMSCRIPTEN
    // Set window position hints if supported
    glfwWindowHint(GLFW_POSITION_X, m_properties.position.x);
    glfwWindowHint(GLFW_POSITION_Y, m_properties.position.y);
#endif

    GLFWmonitor* monitor = nullptr;
    const GLFWvidmode* mode = nullptr;

    // Setup fullscreen if requested
    if (m_properties.isFullscreen)
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

    const int width = m_properties.size.x;
    const int height = m_properties.size.y;

    m_glfwHandle = glfwCreateWindow(width, height, m_properties.title.c_str(), monitor, nullptr);

    if (!m_glfwHandle)
    {
        throw std::runtime_error("Failed to create GLFW window.");
    }

#ifndef MOSAIC_PLATFORM_EMSCRIPTEN
    // Move window if not in fullscreen (fullscreen ignores position)
    if (!m_properties.isFullscreen)
    {
        glfwSetWindowPos(m_glfwHandle, m_properties.position.x, m_properties.position.y);
    }

    // Maximize if requested
    if (m_properties.isMaximized)
    {
        glfwMaximizeWindow(m_glfwHandle);
    }
#endif

    registerCallbacks();

    MOSAIC_DEBUG("Window created: {0} ({1} x {2})", m_properties.title, m_properties.size.x,
                 m_properties.size.y);

    return pieces::OkRef<window::Window, std::string>(*this);
}

void GLFWWindow::shutdown()
{
    if (!m_glfwHandle)
    {
        MOSAIC_ERROR("Window already destroyed.");
        return;
    }

    unregisterCallbacks();

    glfwDestroyWindow(m_glfwHandle);
    m_glfwHandle = nullptr;

    MOSAIC_DEBUG("Window destroyed.");
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
    m_properties.title = _title;
}

void GLFWWindow::setSize(glm::vec2 _size)
{
    glfwSetWindowSize(m_glfwHandle, static_cast<int>(_size.x), static_cast<int>(_size.y));
    m_properties.size = _size;
}

void GLFWWindow::setMinimized(bool _minimized)
{
    _minimized ? glfwIconifyWindow(m_glfwHandle) : glfwRestoreWindow(m_glfwHandle);
    m_properties.isMinimized = _minimized;
}

void GLFWWindow::setMaximized(bool _maximized)
{
    _maximized ? glfwMaximizeWindow(m_glfwHandle) : glfwRestoreWindow(m_glfwHandle);
    m_properties.isMaximized = _maximized;
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

    m_properties.isFullscreen = _fullscreen;
}

void GLFWWindow::setResizeable(bool _resizeable)
{
    glfwSetWindowAttrib(m_glfwHandle, GLFW_RESIZABLE, _resizeable ? GLFW_TRUE : GLFW_FALSE);
    m_properties.isResizeable = _resizeable;
}

void GLFWWindow::setVSync(bool enabled)
{
    enabled ? glfwSwapInterval(1) : glfwSwapInterval(0);
    m_properties.isVSync = enabled;
}

void GLFWWindow::setWindowIcon(const std::string& _path, int _width, int _height)
{
    int w, h, channels;
    unsigned char* data = stbi_load(_path.c_str(), &w, &h, &channels, 4);

    if (!data)
    {
        MOSAIC_ERROR("Failed to load window icon: {0}", _path.c_str());
        return;
    }

    if (_width > 0 && _height > 0 && (_width != w || _height != h))
    {
        unsigned char* resized = (unsigned char*)malloc(_width * _height * 4);

        stbir_resize_uint8_srgb(data, w, h, 0, resized, _width, _height, 0,
                                static_cast<stbir_pixel_layout>(4));

        if (!resized)
        {
            MOSAIC_ERROR("Failed to resize window icon: {0}", _path.c_str());
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
#ifndef MOSAIC_PLATFORM_EMSCRIPTEN
            glfwSetInputMode(m_glfwHandle, GLFW_CURSOR, GLFW_CURSOR_CAPTURED);
#else
            MOSAIC_ERROR("GLFW_CURSOR_CAPTURED is not supported in Emscripten builds.");
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
    if (m_properties.cursorProperties.currentType == _type) return;

    m_properties.cursorProperties.currentType = _type;
    setCursorIcon(m_properties.cursorProperties.srcPaths[static_cast<int>(_type)]);
}

void GLFWWindow::setCursorTypeIcon(window::CursorType _type, const std::string& _path, int _width,
                                   int _height)
{
    if (m_properties.cursorProperties.srcPaths[static_cast<int>(_type)] == _path) return;

    m_properties.cursorProperties.srcPaths[static_cast<int>(_type)] = _path;
    setCursorIcon(_path, _width, _height);
}

void GLFWWindow::setCursorIcon(const std::string& _path, int _width, int _height)
{
    int w, h, channels;
    unsigned char* data = stbi_load(_path.c_str(), &w, &h, &channels, 4);

    if (!data)
    {
        MOSAIC_ERROR("Failed to load cursor icon: {0}", _path.c_str());
        return;
    }

    if (_width > 0 && _height > 0 && (_width != w || _height != h))
    {
        unsigned char* resized = (unsigned char*)malloc(_width * _height * 4);

        stbir_resize_uint8_linear(data, w, h, 0, resized, _width, _height, 0,
                                  static_cast<stbir_pixel_layout>(4));

        if (!resized)
        {
            MOSAIC_ERROR("Failed to resize cursor icon: {0}", _path.c_str());
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
        MOSAIC_ERROR("Failed to create cursor from image: {0}", _path.c_str());
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
        instance->invokeCloseCallbacks();
    }
    else
    {
        MOSAIC_ERROR("GLFWWindow::windowCloseCallback(): callback receiver is null.");
    }
}

void GLFWWindow::windowFocusCallback(GLFWwindow* window, int focused)
{
    GLFWWindow* instance = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));

    if (instance)
    {
        instance->invokeFocusCallbacks(focused);
    }
    else
    {
        MOSAIC_ERROR("GLFWWindow::windowFocusCallback(): callback receiver is null.");
    }
}

void GLFWWindow::windowSizeCallback(GLFWwindow* window, int width, int height)
{
    GLFWWindow* instance = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));

    if (instance)
    {
        instance->m_properties.size = glm::ivec2(width, height);
        instance->invokeResizeCallbacks(width, height);
    }
    else
    {
        MOSAIC_ERROR("GLFWWindow::windowSizeCallback(): callback receiver is null.");
    }
}

void GLFWWindow::windowRefreshCallback(GLFWwindow* window)
{
    GLFWWindow* instance = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));

    if (instance)
    {
        instance->invokeRefreshCallbacks();
    }
    else
    {
        MOSAIC_ERROR("GLFWWindow::windowRefreshCallback(): callback receiver is null.");
    }
}

void GLFWWindow::windowIconifyCallback(GLFWwindow* window, int iconified)
{
    GLFWWindow* instance = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));

    if (instance)
    {
        instance->m_properties.isMinimized = (iconified == GLFW_TRUE);
        instance->invokeIconifyCallbacks(iconified);
    }
    else
    {
        MOSAIC_ERROR("GLFWWindow::windowIconifyCallback(): callback receiver is null.");
    }
}

void GLFWWindow::windowMaximizeCallback(GLFWwindow* window, int maximized)
{
    GLFWWindow* instance = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));

    if (instance)
    {
        instance->m_properties.isMaximized = (maximized == GLFW_TRUE);
        instance->invokeMaximizeCallbacks(maximized);
    }
    else
    {
        MOSAIC_ERROR("GLFWWindow::windowMaximizeCallback(): callback receiver is null.");
    }
}

void GLFWWindow::windowDropCallback(GLFWwindow* window, int count, const char** paths)
{
    GLFWWindow* instance = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));

    if (instance)
    {
        instance->invokeDropCallbacks(count, paths);
    }
    else
    {
        MOSAIC_ERROR("GLFWWindow::windowDropCallback(): callback receiver is null.");
    }
}

void GLFWWindow::windowScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    GLFWWindow* instance = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));

    if (instance)
    {
        instance->invokeScrollCallbacks(xoffset, yoffset);
    }
    else
    {
        MOSAIC_ERROR("GLFWWindow::windowScrollCallback(): callback receiver is null.");
    }
}

void GLFWWindow::windowCursorEnterCallback(GLFWwindow* window, int entered)
{
    GLFWWindow* instance = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));

    if (instance)
    {
        instance->invokeCursorEnterCallbacks(entered);
    }
    else
    {
        MOSAIC_ERROR("GLFWWindow::windowCursorEnterCallback(): callback receiver is null.");
    }
}

void GLFWWindow::windowPosCallback(GLFWwindow* window, int x, int y)
{
    GLFWWindow* instance = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));

    if (instance)
    {
        instance->m_properties.position = glm::ivec2(x, y);
        instance->invokePosCallbacks(x, y);
    }
    else
    {
        MOSAIC_ERROR("GLFWWindow::windowPosCallback(): callback receiver is null.");
    }
}

void GLFWWindow::windowContentScaleCallback(GLFWwindow* window, float xscale, float yscale)
{
    GLFWWindow* instance = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));

    if (instance)
    {
        instance->invokeContentScaleCallbacks(xscale, yscale);
    }
    else
    {
        MOSAIC_ERROR("GLFWWindow::windowContentScaleCallback(): callback receiver is null.");
    }
}

void GLFWWindow::windowCharCallback(GLFWwindow* _window, unsigned int _codepoint)
{
    GLFWWindow* instance = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(_window));

    if (instance)
    {
        instance->invokeCharCallbacks(_codepoint);
    }
    else
    {
        MOSAIC_ERROR("GLFWWindow::windowCharCallback(): callback receiver is null.");
    }
}

} // namespace glfw
} // namespace platform
} // namespace mosaic
