# Package: saturn/platform

**Location:** `saturn/include/saturn/platform/`, `saturn/src/platform/`
**Type:** Part of saturn shared/static library
**Dependencies:** pieces (Result), core/ (Application, System), platform-specific APIs (Win32, POSIX, AGDK, Emscripten, GLFW)

---

## Purpose & Responsibility

### Owns

- Platform abstraction layer (Platform base class)
- Platform-specific lifecycle management (desktop vs web vs mobile)
- Platform services (SysInfo, SysConsole, SysUI)
- Platform context management (PlatformContext)
- Entry point handling (WinMain, main, android_main)
- Five platform implementations (Win32, POSIX, AGDK, Emscripten, GLFW)
- Platform-specific window/input implementations (GLFW, AGDK)
- JNI bridge for Android (AGDK/jni_helper)

### Does NOT Own

- Application logic (core/application.hpp)
- Cross-platform engine systems (graphics, input systems are cross-platform)
- Platform-agnostic window abstraction (window/window.hpp is cross-platform)
- Build system platform selection (CMake handles conditionals)

---

## Key Abstractions & Invariants

### Core Types

- **`Platform`** (`core/platform.hpp:53`) — Base class for platform lifecycle, factory pattern, singleton
- **`PlatformContext`** (`core/platform.hpp:24`) — Platform-specific context resources
- **`SystemInfo`** (`core/sys_info.hpp`) — CPU, GPU, memory queries (CPUInfo, GPUInfo, MemoryInfo)
- **`SystemConsole`** (`core/sys_console.hpp`) — Terminal I/O (create, destroy, attach/detachParent, print)
- **`SystemUI`** (`core/sys_ui.hpp`) — Native dialogs (message boxes, file pickers)

### Platform Implementations (src/platform/)

- **Win32/** — Windows platform (4 files: platform, sys_info, sys_console, sys_ui)
- **POSIX/** — Linux platform (4 files: platform, sys_info, sys_console, sys_ui)
- **AGDK/** — Android platform (8 files: platform, sys_info, sys_console, sys_ui, window, window_system, jni_helper)
- **Emscripten/** — Web platform (4 files: platform, sys_info, sys_console, sys_ui)
- **GLFW/** — Cross-platform windowing (7 files: window, window_system, keyboard/mouse/text input sources)

### Invariants (NEVER violate)

1. **Platform exclusivity**: ONLY one platform active at compile time (Win32 OR POSIX OR AGDK OR Emscripten, never multiple)
2. **GLFW overlay**: GLFW used ON TOP OF Win32/POSIX/Emscripten for desktop windowing (not mutually exclusive)
3. **Lifecycle order**: Platform MUST call: Application::initialize() → update loop → Application::shutdown()
4. **Singleton**: ONLY one Platform instance MUST exist (s_instance static member)
5. **Factory pattern**: Platform MUST be created via Platform::create() (selects platform-specific subclass)
6. **Entry point**: Each platform has distinct entry point (WinMain, main, android_main) - NEVER define multiple
7. **Context callback**: PlatformContext MUST invoke callbacks on context change (e.g., Android activity lifecycle)
8. **System services**: Each platform MUST implement SysInfo, SysConsole, SysUI (abstract base classes)
9. **JNI thread safety**: AGDK JNI calls MUST be made from JNI-attached threads only
10. **Emscripten main loop**: Emscripten MUST use emscripten_set_main_loop() (cannot block in main)

### Architectural Patterns

- **Factory pattern**: Platform::create() returns Win32Platform/POSIXPlatform/AGDKPlatform/EmscriptenPlatform
- **Singleton**: Platform::s_instance for global access
- **Strategy pattern**: Platform subclasses for platform-specific lifecycle
- **Pimpl**: Platform and PlatformContext hide implementation details
- **Service interfaces**: SysInfo, SysConsole, SysUI abstract platform services

---

## Architectural Constraints

### Dependency Rules

**Allowed:**

- pieces (Result)
- core/ (Application, Logger)
- window/ (Window - platform implements WindowSystem)
- input/ (InputSource - platform implements sources)
- Win32 SDK (Windows only)
- POSIX APIs (Linux only)
- AGDK (Android only)
- Emscripten APIs (Web only)
- GLFW (Desktop and Web)

**Forbidden:**

- ❌ graphics/ — Platform does NOT depend on rendering (graphics depends on platform for surface)
- ❌ ecs/ — Platform does NOT depend on ECS
- ❌ Platform APIs in cross-platform code — Isolate in src/platform/<Platform>/

### Layering

- Platform creates and owns Application pointer
- Platform calls Application lifecycle methods (initialize, update, shutdown)
- Application uses Platform services (SysInfo, SysConsole, SysUI via Platform::getInstance())

### Threading Model

- **Platform lifecycle**: Single-threaded (main thread only)
- **SysInfo queries**: Thread-safe (read-only system queries)
- **SysConsole**: Thread-compatible (not thread-safe, caller synchronizes)
- **SysUI dialogs**: Platform-dependent (Win32 thread-safe, X11 not thread-safe)
- **AGDK JNI**: Thread-local (JNI environment per thread)

### Lifetime & Ownership

- **Platform**: Owned by runApp (created in entry point, destroyed at exit)
- **Application**: Owned by runApp, Platform holds raw pointer
- **PlatformContext**: Owned by Platform
- **System services**: Static methods or singletons (SysInfo, SysConsole, SysUI)

### Platform Constraints

- **Win32**: Windows only (MSVC, Clang-CL)
- **POSIX**: Linux only (GCC, Clang)
- **AGDK**: Android only (Clang, NDK r21+)
- **Emscripten**: Web only (Emscripten SDK, LLVM)
- **GLFW**: Desktop (Windows, Linux, macOS), Web (Emscripten)
- **Entry points**: WinMain (Win32), main (POSIX/Emscripten/GLFW), android_main (AGDK)

---

## Modification Rules

### Safe to Change

- Add new platform implementations (macOS, iOS, Switch, etc.)
- Extend SysInfo with more queries (battery, network, thermal)
- Add new SystemUI methods (color pickers, font dialogs)
- Improve SysConsole (color output, cursor control)
- Extend PlatformContext callbacks (orientation change, low memory)

### Requires Coordination

- Changing Platform lifecycle affects Application (core/application.hpp)
- Modifying entry_point.hpp affects all platforms (WinMain, main, android_main)
- Altering PlatformContext API breaks platform context consumers
- Adding platform-specific code requires CMake conditionals

### Almost Never Change

- **Platform factory pattern** — Removing breaks platform selection
- **Lifecycle order** — Application depends on initialize → update → shutdown
- **Entry point macros** — SATURN_ENTRY_POINT is standard creation path
- **Singleton Platform** — Multi-platform instances break global access

---

## Common Pitfalls

### Footguns

- ⚠️ **Using platform APIs in cross-platform code**: Breaks other platforms (isolate in src/platform/<Platform>/)
- ⚠️ **Calling SysConsole before create()**: Undefined behavior (console not initialized)
- ⚠️ **Blocking in Emscripten main()**: Hangs (use emscripten_set_main_loop for update loop)
- ⚠️ **JNI calls from non-JNI threads**: Crashes on Android (attach thread first)
- ⚠️ **Multiple entry points**: Linker error (only define SATURN_ENTRY_POINT once OR android_main)
- ⚠️ **Platform::create() returns nullptr**: Platform not supported (check return value)

### Performance Traps

- 🐌 **Frequent SysInfo queries**: Some queries expensive (CPU topology, GPU info) - cache results
- 🐌 **Blocking dialogs in game loop**: SystemUI::messageBox() blocks rendering (use async dialogs)
- 🐌 **Console output in release**: SysConsole::print() may be slow (disable in release builds)

### Historical Mistakes (Do NOT repeat)

- **Global platform state**: Switched to singleton pattern for controlled access
- **Hard-coded platform selection**: Added factory pattern for runtime/compile-time selection
- **No GLFW overlay**: Added GLFW for cross-platform windowing (reduces per-platform code)

---

## How Claude Should Help

### Expected Tasks

- Add new platform implementations (macOS, iOS, PlayStation, Xbox, Switch)
- Extend SysInfo with platform-specific queries (GPU vendor, driver version, thermal throttling)
- Add async SystemUI dialogs (non-blocking file pickers, progress dialogs)
- Implement platform-specific optimizations (CPU affinity hints, power profiles)
- Write unit tests for SysInfo queries (mock platform data)
- Add platform feature detection (Vulkan support, ray tracing, DLSS)
- Implement clipboard support (SystemClipboard abstraction)

### Conservative Approach Required

- **Changing Platform lifecycle**: Affects Application and entry points (test all platforms)
- **Modifying entry_point.hpp**: Breaks platform-specific builds (verify all 5 platforms compile)
- **Altering PlatformContext**: May break context-dependent code (graphics, window)
- **Adding platform-specific dependencies**: Increases build complexity (justify carefully)

### Before Making Changes

- [ ] Verify changes compile on ALL platforms (Win32, POSIX, AGDK, Emscripten, GLFW)
- [ ] Test platform services (SysInfo, SysConsole, SysUI) on target platform
- [ ] Check entry point works (WinMain, main, android_main)
- [ ] Verify PlatformContext callbacks fire correctly
- [ ] Test AGDK JNI thread safety (attach threads before JNI calls)
- [ ] Confirm Emscripten main loop doesn't block (update loop runs)

---

## Quick Reference

### Files

**Public API:**

- `include/saturn/core/platform.hpp` — Platform, PlatformContext
- `include/saturn/core/sys_info.hpp` — SystemInfo, CPUInfo, GPUInfo, MemoryInfo
- `include/saturn/core/sys_console.hpp` — SystemConsole (create, destroy, print, attach/detachParent)
- `include/saturn/core/sys_ui.hpp` — SystemUI (messageBox, file dialogs)
- `include/saturn/platform/Win32/wstring.hpp` — Wide string conversion (Windows)
- `include/saturn/platform/AGDK/jni_helper.hpp` — JNI utilities (Android)
- `include/saturn/platform/AGDK/agdk_platform.hpp` — AGDK platform public API
- `include/saturn/platform/GLFW/glfw_input_mappings.hpp` — GLFW key/button mappings

**Platform Implementations:**

- `src/platform/Win32/` — Windows (4 files)
- `src/platform/POSIX/` — Linux (4 files)
- `src/platform/AGDK/` — Android (8 files)
- `src/platform/Emscripten/` — Web (4 files)
- `src/platform/GLFW/` — Cross-platform windowing (7 files)

**Tests:**

- None currently (tests needed for SysInfo queries, platform lifecycle)

### Key Functions/Methods

- `Platform::create(Application*)` → unique_ptr<Platform> — Factory for platform-specific subclass
- `Platform::initialize()` → RefResult<Platform, string> — Initialize platform and application
- `Platform::run()` → RefResult<Platform, string> — Run main loop (calls Application::update())
- `Platform::shutdown()` → Cleanup platform and application
- `SystemConsole::create()` — Create console window (Windows)
- `SystemConsole::print(msg)` — Print to console
- `SystemInfo::getCPUInfo()` → CPUInfo — Query CPU information
- `SystemUI::messageBox(title, message)` — Show native message box

### Build Flags

- `SATURN_PLATFORM_WIN32` — Windows platform
- `SATURN_PLATFORM_POSIX` — Linux platform
- `SATURN_PLATFORM_ANDROID` — Android platform (AGDK)
- `SATURN_PLATFORM_EMSCRIPTEN` — Web platform

---

## Status Notes

**Stable** — Five platform implementations functional (Win32, POSIX, AGDK, Emscripten, GLFW). macOS/iOS not implemented.
