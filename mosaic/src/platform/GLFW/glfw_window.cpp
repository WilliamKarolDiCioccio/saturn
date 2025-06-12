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

GLFWWindow::GLFWWindow(const std::string& _title, glm::ivec2 _size)
    : m_glfwHandle(nullptr), core::Window(_title, _size)
{
#ifdef MOSAIC_PLATFORM_EMSCRIPTEN
    static int windowCount = 0;

    if (windowCount > 0)
    {
        throw std::runtime_error("Only one GLFW window is allowed in Emscripten builds.");
    }

    windowCount++;
#endif

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
    glfwWindowHint(GLFW_FOCUSED, GLFW_TRUE);
    glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);

#ifndef MOSAIC_PLATFORM_EMSCRIPTEN
    glfwWindowHint(GLFW_POSITION_X, m_properties.position.x);
    glfwWindowHint(GLFW_POSITION_Y, m_properties.position.y);
#endif

    m_glfwHandle = glfwCreateWindow(_size.x, _size.y, _title.c_str(), nullptr, nullptr);

    if (!m_glfwHandle)
    {
        throw std::runtime_error("Failed to create GLFW window.");
    }

    registerCallbacks();

    MOSAIC_DEBUG("Window created: {0} ({1} x {2})", _title, _size.x, _size.y);
}

GLFWWindow::~GLFWWindow()
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

void GLFWWindow::setCursorMode(core::CursorMode _mode)
{
    switch (_mode)
    {
        case core::CursorMode::normal:
            glfwSetInputMode(m_glfwHandle, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            break;
        case core::CursorMode::captured:
#ifndef __EMSCRIPTEN__
            glfwSetInputMode(m_glfwHandle, GLFW_CURSOR, GLFW_CURSOR_CAPTURED);
#endif
            break;
        case core::CursorMode::hidden:
            glfwSetInputMode(m_glfwHandle, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
            break;
        case core::CursorMode::disabled:
            glfwSetInputMode(m_glfwHandle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            break;
    }
}

void GLFWWindow::setCursorType(core::CursorType _type)
{
    if (m_properties.cursorProperties.currentType == _type) return;

    m_properties.cursorProperties.currentType = _type;
    setCursorIcon(m_properties.cursorProperties.srcPaths[static_cast<int>(_type)]);
}

void GLFWWindow::setCursorTypeIcon(core::CursorType _type, const std::string& _path, int _width,
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

} // namespace glfw
} // namespace platform
} // namespace mosaic
