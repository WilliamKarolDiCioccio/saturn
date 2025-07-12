#pragma once

#include <string>
#include <array>
#include <functional>
#include <vector>

#include <pieces/result.hpp>

#include <glm/glm.hpp>

#include "mosaic/internal/defines.hpp"

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

template <typename Fn>
struct WindowEventCallback
{
    using CallbackFn = Fn;
    Fn callback;
    size_t id;

    WindowEventCallback(size_t _id, const Fn& _callback)
        : id(_id), callback(std::move(_callback)) {};
};

// Callback type definitions

using WindowCloseCallbackFn = std::function<void()>;
using WindowFocusCallbackFn = std::function<void(int)>;
using WindowResizeCallbackFn = std::function<void(int, int)>;
using WindowRefreshCallbackFn = std::function<void()>;
using WindowIconifyCallbackFn = std::function<void(int)>;
using WindowMaximizeCallbackFn = std::function<void(int)>;
using WindowDropCallbackFn = std::function<void(int, const char**)>;
using WindowScrollCallbackFn = std::function<void(double, double)>;
using WindowCursorEnterCallbackFn = std::function<void(int)>;
using WindowPosCallbackFn = std::function<void(int, int)>;
using WindowContentScaleCallbackFn = std::function<void(float, float)>;
using WindowCharCallbackFn = std::function<void(unsigned int)>;

using WindowCloseCallback = WindowEventCallback<WindowCloseCallbackFn>;
using WindowFocusCallback = WindowEventCallback<WindowFocusCallbackFn>;
using WindowResizeCallback = WindowEventCallback<WindowResizeCallbackFn>;
using WindowRefreshCallback = WindowEventCallback<WindowRefreshCallbackFn>;
using WindowIconifyCallback = WindowEventCallback<WindowIconifyCallbackFn>;
using WindowMaximizeCallback = WindowEventCallback<WindowMaximizeCallbackFn>;
using WindowDropCallback = WindowEventCallback<WindowDropCallbackFn>;
using WindowScrollCallback = WindowEventCallback<WindowScrollCallbackFn>;
using WindowCursorEnterCallback = WindowEventCallback<WindowCursorEnterCallbackFn>;
using WindowPosCallback = WindowEventCallback<WindowPosCallbackFn>;
using WindowContentScaleCallback = WindowEventCallback<WindowContentScaleCallbackFn>;
using WindowCharCallback = WindowEventCallback<WindowCharCallbackFn>;

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
    [[nodiscard]] virtual glm::ivec2 getFramebufferSize() const = 0;

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
    [[nodiscard]] virtual std::string getClipboardString() const = 0;

    // Property getters
    [[nodiscard]] inline const WindowProperties& getWindowProperties() const
    {
        return m_properties;
    }

    [[nodiscard]] inline const CursorProperties& getCursorProperties() const
    {
        return m_properties.cursorProperties;
    }

    // Callback registration methods

    template <typename T>
    [[nodiscard]] inline auto getCallbackVector() -> std::vector<T>&
    {
        if constexpr (std::is_same_v<T, WindowCloseCallback>)
        {
            return m_windowCloseCallbacks;
        }
        else if constexpr (std::is_same_v<T, WindowFocusCallback>)
        {
            return m_windowFocusCallbacks;
        }
        else if constexpr (std::is_same_v<T, WindowResizeCallback>)
        {
            return m_windowResizeCallbacks;
        }
        else if constexpr (std::is_same_v<T, WindowRefreshCallback>)
        {
            return m_windowRefreshCallbacks;
        }
        else if constexpr (std::is_same_v<T, WindowIconifyCallback>)
        {
            return m_windowIconifyCallbacks;
        }
        else if constexpr (std::is_same_v<T, WindowMaximizeCallback>)
        {
            return m_windowMaximizeCallbacks;
        }
        else if constexpr (std::is_same_v<T, WindowDropCallback>)
        {
            return m_windowDropCallbacks;
        }
        else if constexpr (std::is_same_v<T, WindowScrollCallback>)
        {
            return m_windowScrollCallbacks;
        }
        else if constexpr (std::is_same_v<T, WindowCursorEnterCallback>)
        {
            return m_windowCursorEnterCallbacks;
        }
        else if constexpr (std::is_same_v<T, WindowPosCallback>)
        {
            return m_windowPosCallbacks;
        }
        else if constexpr (std::is_same_v<T, WindowContentScaleCallback>)
        {
            return m_windowContentScaleCallbacks;
        }
        else if constexpr (std::is_same_v<T, WindowCharCallback>)
        {
            return m_windowCharCallbacks;
        }
        else
        {
            static_assert(false, "Unsupported callback type");
        }
    }

    template <typename T>
    [[nodiscard]] size_t registerWindowCallback(const typename T::CallbackFn& _callback)
    {
        static std::atomic<size_t> callbackIdCounter(0);

        callbackIdCounter++;

        auto& callbacks = getCallbackVector<T>();
        callbacks.emplace_back(callbackIdCounter.load(), _callback);

        return callbackIdCounter.load();
    }

    template <typename T>
    void unregisterWindowCallback(size_t _id)
    {
        auto& callbacks = getCallbackVector<T>();
        auto it = std::remove_if(callbacks.begin(), callbacks.end(),
                                 [_id](const T& cb) { return cb.id == _id; });
        if (it != callbacks.end()) callbacks.erase(it);
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
