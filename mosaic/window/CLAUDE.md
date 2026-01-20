# Package: saturn/window

**Location:** `saturn/include/saturn/window/`, `saturn/src/window/`
**Type:** Part of saturn shared/static library
**Dependencies:** pieces (Result), core/ (System), glm (math), platform-specific windowing (GLFW, AGDK)

---

## Purpose & Responsibility

### Owns

- Cross-platform window abstraction (Window, WindowSystem)
- Window creation, destruction, and lifecycle management
- Window properties (title, size, position, fullscreen, VSync, resizability)
- Cursor management (type, mode, visibility, clipping)
- Multi-window support (windows identified by string ID)
- Window event callbacks (resize, focus, close, etc.)
- Platform-specific window implementations (GLFW, AGDK)

### Does NOT Own

- Platform selection logic (platform/ package handles this)
- Input handling beyond window focus (input/ package)
- Rendering to window (graphics/ package creates RenderContext for window)
- Window surface creation for graphics (graphics/ handles VkSurfaceKHR)

---

## Key Abstractions & Invariants

### Core Types

- **`WindowSystem`** (`window_system.hpp:33`) — EngineSystem, manages windows, singleton, factory pattern
- **`Window`** (`window.hpp`) — Window instance, Pimpl pattern
- **`WindowProperties`** (`window.hpp:75`) — title, size, position, fullscreen, minimized, maximized, resizeable, VSync, cursorProperties
- **`CursorProperties`** (`window.hpp:56`) — currentType, currentMode, srcPaths, isVisible, isClipped
- **`CursorType`** (`window.hpp:25`) — Enum: arrow, hand, text, crosshair, resize (ns/we/nwse/nesw), i_beam, custom
- **`CursorMode`** (`window.hpp:45`) — Enum: normal, captured, hidden, disabled

### Platform Implementations (src/platform/)

- **GLFW** (`src/platform/GLFW/glfw_window.cpp`, `glfw_window_system.cpp`) — Desktop (Windows/Linux/macOS), Web (Emscripten)
- **AGDK** (`src/platform/AGDK/agdk_window.cpp`, `agdk_window_system.cpp`) — Android (native activity)

### Invariants (NEVER violate)

1. **Window ID uniqueness**: Window IDs MUST be unique within WindowSystem (string-based map)
2. **One system instance**: WindowSystem singleton MUST have exactly one instance (g_instance)
3. **Factory creation**: WindowSystem MUST be created via WindowSystem::create() (returns platform-specific subclass)
4. **Event poll order**: WindowSystem::update() MUST be called FIRST in main loop (before input, render)
5. **Window ownership**: WindowSystem owns all Window instances (unique_ptr), destroyed on destroyWindow() or shutdown()
6. **Platform exclusivity**: GLFW OR AGDK implementation active at compile time (never both)
7. **Cursor clipping**: Cursor clipping (confined to window) platform-dependent (Windows supports, others may not)
8. **VSync control**: VSync controlled via WindowProperties, enforced by graphics swapchain
9. **Size constraints**: Window size MUST be > 0 (width, height positive)
10. **Update before render**: WindowSystem::update() polls events, MUST complete before RenderContext::render()

### Architectural Patterns

- **Factory pattern**: WindowSystem::create() returns GLFWWindowSystem or AGDKWindowSystem
- **Singleton**: WindowSystem::g_instance for global access
- **Pimpl**: Window hides platform-specific window handle (GLFWwindow*, ANativeWindow*)
- **String-based IDs**: Windows identified by string keys (enables named window lookup)

---

## Architectural Constraints

### Dependency Rules

**Allowed:**

- pieces (Result)
- core/ (System lifecycle, Logger)
- glm (math for size, position)
- GLFW (desktop/web windowing)
- AGDK (Android windowing)

**Forbidden:**

- ❌ graphics/ — Window does NOT depend on rendering (graphics depends on window)
- ❌ input/ — Window does NOT depend on input (input depends on window)
- ❌ Direct platform window APIs — Use WindowSystem abstraction

### Layering

- Application owns WindowSystem (EngineSystem)
- WindowSystem creates Window instances
- RenderSystem creates RenderContext for Window
- InputSystem creates InputContext for Window

### Threading Model

- **WindowSystem**: Single-threaded (main thread only)
- **Window**: Single-threaded (not thread-safe)
- **GLFW**: Main thread only (GLFW restriction)
- **AGDK**: Main thread only (Android UI thread restriction)

### Lifetime & Ownership

- **WindowSystem**: Owned by Application, singleton g_instance
- **Window**: Owned by WindowSystem via unique_ptr (destroyed on destroyWindow())
- **Event callbacks**: Owned by Window (function<> stored in Window::Impl)

### Platform Constraints

- **GLFW**: Desktop (Windows, Linux, macOS), Web (Emscripten)
- **AGDK**: Android only (native activity)
- **Cursor control**: Full cursor control on desktop, limited on mobile/web
- **Multi-window**: GLFW supports multi-window, AGDK single-window only (Android limitation)

---

## Modification Rules

### Safe to Change

- Add new CursorType entries (resize modes, custom cursors)
- Extend WindowProperties (DPI scaling, borderless, always-on-top)
- Add window event callbacks (DPI changed, moved, state changed)
- Improve cursor clipping (virtual desktop multi-monitor support)
- Add window decorations control (hide title bar, custom window frame)

### Requires Coordination

- Changing WindowSystem lifecycle affects Application::update() order
- Modifying Window API breaks graphics/ and input/ integration
- Altering WindowProperties structure breaks window creation call sites
- Adding platform-specific windows requires CMake conditionals

### Almost Never Change

- **Factory pattern** — WindowSystem::create() is standard creation path
- **String-based IDs** — Switching to numeric IDs breaks named window lookup
- **Singleton pattern** — Multi-instance WindowSystem breaks global access
- **Event poll first** — Moving update() later in loop increases input latency

---

## Common Pitfalls

### Footguns

- ⚠️ **Calling update() after input poll**: Stale input data (window events not yet processed)
- ⚠️ **Creating window with size (0, 0)**: Invalid window (check WindowProperties)
- ⚠️ **Duplicate window IDs**: createWindow() may overwrite existing window (check getWindow() first)
- ⚠️ **Destroying window while rendering**: RenderContext holds Window\* (destroy context first)
- ⚠️ **Cursor clipping on non-Windows**: May not work (platform limitation)
- ⚠️ **Multi-window on AGDK**: Not supported (Android single-activity model)

### Performance Traps

- 🐌 **Frequent resize**: Triggers swapchain recreation (expensive - debounce resize events)
- 🐌 **Window property queries**: Some queries poll OS (cache window size/position)
- 🐌 **Event callbacks with heavy logic**: Blocks event loop (defer work to update())

### Historical Mistakes (Do NOT repeat)

- **Direct GLFW usage in core code**: Abstracted to WindowSystem (cross-platform)
- **Numeric window IDs**: Switched to string IDs (named window lookup)
- **No multi-window support**: Added multi-window for editor use cases

---

## How Claude Should Help

### Expected Tasks

- Add window event callbacks (move, resize, focus, minimize, maximize, close)
- Implement DPI scaling support (high-DPI displays, per-monitor DPI awareness)
- Add borderless windowed mode (fullscreen without exclusive mode)
- Extend cursor control (lock to window center for FPS games)
- Implement window decorations control (hide title bar, custom frame)
- Add clipboard integration (copy/paste text, images)
- Write unit tests for window lifecycle (create, resize, destroy)

### Conservative Approach Required

- **Changing WindowSystem API**: Affects Application, graphics, input integration
- **Modifying Window lifecycle**: May break RenderContext/InputContext creation
- **Altering WindowProperties**: Breaks window creation call sites
- **Removing platform implementations**: Breaks platform builds (GLFW or AGDK)

### Before Making Changes

- [ ] Verify changes compile on platforms (GLFW: Windows/Linux/Web, AGDK: Android)
- [ ] Test multi-window support (createWindow multiple times, destroyWindow)
- [ ] Check window resize triggers swapchain recreation (graphics integration)
- [ ] Verify input context updates after window events (input integration)
- [ ] Test cursor modes (normal, captured, hidden, disabled)
- [ ] Confirm VSync control works (graphics swapchain respects WindowProperties)

---

## Quick Reference

### Files

**Public API:**

- `include/saturn/window/window.hpp` — Window, WindowProperties, CursorProperties, CursorType, CursorMode
- `include/saturn/window/window_system.hpp` — WindowSystem

**Internal:**

- `src/window/window.cpp` — Window base implementation
- `src/window/window_system.cpp` — WindowSystem base implementation

**Platform-Specific:**

- `src/platform/GLFW/glfw_window.cpp` — GLFW Window implementation
- `src/platform/GLFW/glfw_window_system.cpp` — GLFW WindowSystem implementation
- `src/platform/AGDK/agdk_window.cpp` — AGDK Window implementation
- `src/platform/AGDK/agdk_window_system.cpp` — AGDK WindowSystem implementation

**Tests:**

- None currently (tests needed for window lifecycle, multi-window, resize)

### Key Functions/Methods

- `WindowSystem::create()` → unique_ptr<WindowSystem> — Factory for platform-specific window system
- `WindowSystem::createWindow(id, properties)` → Result<Window\*, string> — Create window
- `WindowSystem::destroyWindow(id)` — Destroy window by ID
- `WindowSystem::update()` → RefResult<System, string> — Poll window events (GLFW pollEvents)
- `Window::setTitle(title)` — Change window title
- `Window::resize(width, height)` — Resize window
- `Window::setFullscreen(fullscreen)` — Toggle fullscreen
- `Window::setCursorMode(mode)` — Set cursor mode (normal, captured, hidden, disabled)

### Build Flags

- None (platform window system selected via CMake conditionals)

---

## Status Notes

**Stable** — GLFW and AGDK implementations functional. Multi-window supported (GLFW only). DPI scaling and window decorations control not implemented.
