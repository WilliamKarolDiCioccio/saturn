#pragma once

#include <string>
#include <array>
#include <functional>
#include <vector>

#include <pieces/result.hpp>

#include <glm/glm.hpp>

#include "mosaic/defines.hpp"

namespace mosaic
{
namespace window
{

/**
 * @brief Enumeration for different cursor types.
 *
 * It provides mapping between the cursor type and the corresponding icon.
 * This allows for easy customization of the cursor appearance based on the context.
 */
enum class CursorType
{
    arrow,
    hand,
    text,
    crosshair,
    resize_ns,
    resize_we,
    resize_nwse,
    resize_nesw,
    i_beam,
    custom
};

/**
 * @brief Enumeration for different cursor modes.
 *
 * It defines the visibility and interaction mode of the cursor.
 * This allows for better control over the cursor behavior in different contexts.
 */
enum class CursorMode
{
    normal,
    captured,
    hidden,
    disabled
};

/**
 * @brief Structure to hold properties related to the cursor.
 */
struct CursorProperties
{
    CursorType currentType;
    CursorMode currentMode;
    std::array<std::string, 10> srcPaths;
    bool isVisible;
    bool isClipped;

    CursorProperties()
        : currentType(CursorType::arrow),
          currentMode(CursorMode::normal),
          srcPaths(),
          isVisible(true),
          isClipped(false) {};
};

/**
 * @brief Structure to hold properties related to the window.
 */
struct WindowProperties
{
    std::string title;
    glm::ivec2 size;
    glm::ivec2 position;
    bool isFullscreen;
    bool isMinimized;
    bool isMaximized;
    bool isResizeable;
    bool isVSync;
    CursorProperties cursorProperties;

    WindowProperties()
        : title("Game Window"),
          size({1280, 720}),
          position({100, 100}),
          isFullscreen(false),
          isMinimized(false),
          isMaximized(false),
          isResizeable(true),
          isVSync(false),
          cursorProperties() {};
};

/**
 * @brief Abstract base class representing a window in the application.
 *
 * This class provides a generic interface for window management that can be
 * implemented by different windowing backends (GLFW, SDL, etc.).
 *
 * It keeps track of the window properties and cursor properties and allows to manipulate them
 * easily.
 *
 * @note This class is not meant to be instantiated directly. Use the static method
 * `create()` to obtain an instance.
 *
 * @see WindowProperties
 * @see CursorProperties
 */
class MOSAIC_API Window
{
   public:
    // Callback type definitions
    using WindowCloseCallback = std::function<void()>;
    using WindowFocusCallback = std::function<void(int)>;
    using WindowResizeCallback = std::function<void(int, int)>;
    using WindowRefreshCallback = std::function<void()>;
    using WindowIconifyCallback = std::function<void(int)>;
    using WindowMaximizeCallback = std::function<void(int)>;
    using WindowDropCallback = std::function<void(int, const char**)>;
    using WindowScrollCallback = std::function<void(double, double)>;
    using WindowCursorEnterCallback = std::function<void(int)>;
    using WindowPosCallback = std::function<void(int, int)>;
    using WindowContentScaleCallback = std::function<void(float, float)>;
    using WindowCharCallback = std::function<void(unsigned int)>;

   protected:
    WindowProperties m_properties;

    // Callback storage
    std::vector<WindowCloseCallback> m_windowCloseCallbacks;
    std::vector<WindowFocusCallback> m_windowFocusCallbacks;
    std::vector<WindowResizeCallback> m_windowResizeCallbacks;
    std::vector<WindowRefreshCallback> m_windowRefreshCallbacks;
    std::vector<WindowIconifyCallback> m_windowIconifyCallbacks;
    std::vector<WindowMaximizeCallback> m_windowMaximizeCallbacks;
    std::vector<WindowDropCallback> m_windowDropCallbacks;
    std::vector<WindowScrollCallback> m_windowScrollCallbacks;
    std::vector<WindowCursorEnterCallback> m_windowCursorEnterCallbacks;
    std::vector<WindowPosCallback> m_windowPosCallbacks;
    std::vector<WindowContentScaleCallback> m_windowContentScaleCallbacks;
    std::vector<WindowCharCallback> m_windowCharCallbacks;

   public:
    Window() = default;
    virtual ~Window() = default;

    static std::unique_ptr<Window> create();

   public:
    virtual pieces::RefResult<Window, std::string> initialize(
        const WindowProperties& _properties) = 0;
    virtual void shutdown() = 0;

    // Fundamental window operations
    virtual void* getNativeHandle() const = 0;
    virtual bool shouldClose() const = 0;
    virtual glm::ivec2 getFramebufferSize() const = 0;

    // Window properties management
    virtual void setTitle(const std::string& _title) = 0;
    virtual void setMinimized(bool _minimized) = 0;
    virtual void setMaximized(bool _maximized) = 0;
    virtual void setFullscreen(bool _fullscreen) = 0;
    virtual void setSize(glm::vec2 _size) = 0;
    virtual void setResizeable(bool _resizeable) = 0;
    virtual void setVSync(bool _enabled) = 0;
    virtual void setWindowIcon(const std::string& _path, int _width = 0, int _height = 0) = 0;
    virtual void resetWindowIcon() = 0;

    // Cursor properties management
    virtual void setCursorMode(CursorMode _mode) = 0;
    virtual void setCursorType(CursorType _type) = 0;
    virtual void setCursorTypeIcon(CursorType _type, const std::string& _path, int _width = 0,
                                   int _height = 0) = 0;

    // Direct manipulation of cursor properties
    virtual void setCursorIcon(const std::string& _path, int _width = 0, int _height = 0) = 0;
    virtual void resetCursorIcon() = 0;

    // Clipboard operations
    virtual void setClipboardString(const std::string& _string) = 0;
    virtual std::string getClipboardString() const = 0;

    // Property getters
    inline const WindowProperties& getWindowProperties() const { return m_properties; }

    inline const CursorProperties& getCursorProperties() const
    {
        return m_properties.cursorProperties;
    }

    // Callback registration methods
    inline void registerWindowCloseCallback(WindowCloseCallback _callback)
    {
        m_windowCloseCallbacks.push_back(_callback);
    }

    inline void registerWindowFocusCallback(WindowFocusCallback _callback)
    {
        m_windowFocusCallbacks.push_back(_callback);
    }

    inline void registerWindowResizeCallback(WindowResizeCallback callback)
    {
        m_windowResizeCallbacks.push_back(callback);
    }

    inline void registerWindowRefreshCallback(WindowRefreshCallback _callback)
    {
        m_windowRefreshCallbacks.push_back(_callback);
    }

    inline void registerWindowIconifyCallback(WindowIconifyCallback _callback)
    {
        m_windowIconifyCallbacks.push_back(_callback);
    }

    inline void registerWindowMaximizeCallback(WindowMaximizeCallback _callback)
    {
        m_windowMaximizeCallbacks.push_back(_callback);
    }

    inline void registerWindowDropCallback(WindowDropCallback _callback)
    {
        m_windowDropCallbacks.push_back(_callback);
    }

    inline void registerWindowScrollCallback(WindowScrollCallback _callback)
    {
        m_windowScrollCallbacks.push_back(_callback);
    }

    inline void registerWindowCursorEnterCallback(WindowCursorEnterCallback _callback)
    {
        m_windowCursorEnterCallbacks.push_back(_callback);
    }

    inline void registerWindowPosCallback(WindowPosCallback _callback)
    {
        m_windowPosCallbacks.push_back(_callback);
    }

    inline void registerWindowContentScaleCallback(WindowContentScaleCallback _callback)
    {
        m_windowContentScaleCallbacks.push_back(_callback);
    }

    inline void registerWindowCharCallback(WindowCharCallback _callback)
    {
        m_windowCharCallbacks.push_back(_callback);
    }

   protected:
    // Helper methods for derived classes to invoke callbacks
    void invokeCloseCallbacks();
    void invokeFocusCallbacks(int _focused);
    void invokeResizeCallbacks(int _width, int _height);
    void invokeRefreshCallbacks();
    void invokeIconifyCallbacks(int _iconified);
    void invokeMaximizeCallbacks(int _maximized);
    void invokeDropCallbacks(int _count, const char** _paths);
    void invokeScrollCallbacks(double _xoffset, double _yoffset);
    void invokeCursorEnterCallbacks(int _entered);
    void invokePosCallbacks(int _x, int _y);
    void invokeContentScaleCallbacks(float _xscale, float _yscale);
    void invokeCharCallbacks(unsigned int _codepoint);
};

} // namespace window
} // namespace mosaic
