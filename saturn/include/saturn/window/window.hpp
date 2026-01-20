#pragma once

#include <string>
#include <array>
#include <functional>
#include <vector>

#include <pieces/core/result.hpp>

#include <glm/glm.hpp>

#include "saturn/defines.hpp"

namespace saturn
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

enum class WindowCallbackType
{
    close,
    focus,
    resize,
    refresh,
    iconify,
    maximize,
    drop,
    scroll,
    cursorEnter,
    pos,
    contentScale,
    character
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
class SATURN_API Window
{
   private:
    struct Impl;

    Impl* m_impl;

   public:
    Window();
    virtual ~Window();

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
    [[nodiscard]] const WindowProperties getWindowProperties() const;
    [[nodiscard]] const CursorProperties getCursorProperties() const;

#define DEFINE_CALLBACK(_Type, _Name, _Params, _Args)              \
    size_t register##_Name##Callback(const _Type::CallbackFn& cb); \
    void unregister##_Name##Callback(size_t id);                   \
    std::vector<_Type> get##_Name##Callbacks();                    \
    void invoke##_Name##Callbacks _Params;

#include "callbacks.def"

#undef DEFINE_CALLBACK

   protected:
    [[nodiscard]] WindowProperties& getWindowPropertiesInternal();
    [[nodiscard]] CursorProperties& getCursorPropertiesInternal();
};

} // namespace window
} // namespace saturn
