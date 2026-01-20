#pragma once

#include "saturn/window/window.hpp"

#include <GLFW/glfw3.h>

namespace saturn
{
namespace platform
{
namespace glfw
{

/**
 * @brief GLFW-specific implementation of the Window class.
 *
 * This class provides concrete implementation of window functionality using GLFW.
 * It handles GLFW-specific operations and maintains the GLFW window handle.
 */
class SATURN_API GLFWWindow : public window::Window
{
   private:
    GLFWwindow* m_glfwHandle;

   public:
    ~GLFWWindow() override = default;

    pieces::RefResult<window::Window, std::string> initialize(
        const window::WindowProperties& _properties) override;
    void shutdown() override;

    void* getNativeHandle() const override;
    bool shouldClose() const override;
    glm::ivec2 getFramebufferSize() const override;

    void setTitle(const std::string& _title) override;
    void setMinimized(bool _minimized) override;
    void setMaximized(bool _maximized) override;
    void setFullscreen(bool _fullscreen) override;
    void setSize(glm::vec2 _size) override;
    void setResizeable(bool _resizeable) override;
    void setVSync(bool _enabled) override;

    void setCursorMode(window::CursorMode _mode) override;
    void setCursorType(window::CursorType _type) override;
    void setCursorTypeIcon(window::CursorType _type, const std::string& _path, int _width = 0,
                           int _height = 0) override;

    void setCursorIcon(const std::string& _path, int _width = 0, int _height = 0) override;
    void resetCursorIcon() override;
    void setWindowIcon(const std::string& _path, int _width = 0, int _height = 0) override;
    void resetWindowIcon() override;

    void setClipboardString(const std::string& _string) override;
    std::string getClipboardString() const override;

   private:
    void registerCallbacks();
    void unregisterCallbacks();

    static void windowCloseCallback(GLFWwindow* _window);
    static void windowFocusCallback(GLFWwindow* _window, int _focused);
    static void windowSizeCallback(GLFWwindow* _window, int _width, int _height);
    static void windowRefreshCallback(GLFWwindow* _window);
    static void windowIconifyCallback(GLFWwindow* _window, int _iconified);
    static void windowMaximizeCallback(GLFWwindow* _window, int _maximized);
    static void windowDropCallback(GLFWwindow* _window, int _count, const char** _paths);
    static void windowScrollCallback(GLFWwindow* _window, double _xoffset, double _yoffset);
    static void windowCursorEnterCallback(GLFWwindow* _window, int _entered);
    static void windowPosCallback(GLFWwindow* _window, int _x, int _y);
    static void windowContentScaleCallback(GLFWwindow* _window, float _xscale, float _yscale);
    static void windowCharCallback(GLFWwindow* _window, unsigned int _codepoint);
};

} // namespace glfw
} // namespace platform
} // namespace saturn
