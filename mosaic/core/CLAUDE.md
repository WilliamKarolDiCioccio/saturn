# Package: saturn/core

**Location:** `saturn/include/saturn/core/`, `saturn/src/core/`
**Type:** Part of saturn shared/static library
**Dependencies:** pieces, fmt, moodycamel queues (readerwriterqueue, concurrentqueue)

---

## Purpose & Responsibility

### Owns

- Application lifecycle management (Application class)
- System registry and lifecycle orchestration (System, EngineSystem, ClientSystem)
- Application state machine (uninitialized → initialized → resumed ⇄ paused → shutdown)
- Platform abstraction layer (Platform base class, platform-specific implementations in platform/)
- Entry point macros (SATURN_ENTRY_POINT, runApp template)
- Event bus with thread-safe pub-sub (EventBus, EventEmitter, EventReceiver)
- System services (Logger, Tracer, CommandLineParser singletons)
- Platform services (SystemConsole, SystemUI, SystemInfo)
- Timing utilities (Timer)

### Does NOT Own

- Window creation/management (window/ package)
- Input handling beyond platform abstraction (input/ package)
- Rendering (graphics/ package)
- ECS entity/component management (ecs/ package)
- Multi-threaded execution (exec/ package)
- Scene graph (scene/ package)

---

## Key Abstractions & Invariants

### Core Types

- **`Application`** (`application.hpp:60`) — Base class for apps, owns engine systems, Pimpl pattern
- **`ApplicationState`** (`application.hpp:32`) — State enum: uninitialized, initialized, resumed, paused, shutdown
- **`System`** (`system.hpp:36`) — Base interface for all systems, lifecycle methods
- **`EngineSystem`** (`system.hpp:80`) — Base for internal systems (window, input, render, scene)
- **`EngineSystemType`** (`system.hpp:64`) — Enum: window, input, render, scene
- **`Platform`** (`platform.hpp`) — Platform abstraction, created via factory pattern
- **`EventBus`** (`events.hpp:97`) — Thread-safe pub-sub with lock-free queues, shared_ptr-based
- **`EventEmitter`** (`events.hpp:452`) — MPMC emitter (alias for EventEmitterBase<ConcurrentQueue>)
- **`EventReceiver`** (`events.hpp:335`) — RAII subscription manager, auto-disconnects on destruction
- **`Subscription`** (`events.hpp:33`) — Token for event subscription with disconnect()

### Invariants (NEVER violate)

1. **State machine order**: Application MUST transition: uninitialized → initialize() → initialized → resume() → resumed ⇄ pause()/paused → shutdown() → shutdown
2. **System lifecycle**: Systems MUST NOT skip states (cannot resume() before initialize())
3. **Entry point singularity**: ONLY use SATURN_ENTRY_POINT macro once per application (defines main/WinMain/android_main)
4. **Application non-copyable**: Application MUST be non-copyable, non-movable (Pimpl pattern)
5. **System ownership**: Application owns EngineSystem instances (WindowSystem, InputSystem, RenderSystem)
6. **Railway-Oriented**: ALL lifecycle methods (initialize, update) MUST return RefResult<T, string> (never throw)
7. **EventBus thread-safety**: EventBus MUST be thread-safe (reader-writer locks + lock-free queues)
8. **Subscription lifetime**: Subscriptions MUST be disconnected before EventBus destruction (use EventReceiver RAII)
9. **Singleton initialization order**: Logger → Tracer → CommandLineParser (enforced in runApp)
10. **Platform factory**: Platform MUST be created via Platform::create() (factory pattern, not direct construction)

### Architectural Patterns

- **Pimpl**: Application hides implementation details via forward-declared Impl struct
- **State machine**: ApplicationState + SystemState with explicit transition methods
- **Factory pattern**: Platform::create() selects platform-specific implementation
- **Template entry point**: SATURN_ENTRY_POINT(AppType) expands to platform-specific main()
- **Railway-Oriented Programming**: RefResult<System, string> for chained lifecycle calls
- **Pub-Sub**: EventBus with type-erased queues, templated subscribe<Event>(fn)
- **Singleton**: Logger, Tracer, CommandLineParser (static getInstance())
- **RAII subscriptions**: EventReceiver auto-disconnects on destruction

---

## Architectural Constraints

### Dependency Rules

**Allowed:**

- pieces (Result<T, E>, containers, string utilities)
- fmt (logging)
- moodycamel queues (readerwriterqueue, concurrentqueue for EventBus)
- window/ (Application owns WindowSystem)
- input/ (Application owns InputSystem)
- graphics/ (Application owns RenderSystem)
- scene/ (Application owns SceneSystem - future)

**Forbidden:**

- ❌ ecs/ → core/ — ECS does NOT depend on core (ecs is lower-level)
- ❌ exec/ → core/ — ThreadPool independent of Application
- ❌ Circular dependency — core owns engine systems, systems MUST NOT own Application

### Layering

- Application orchestrates systems (owns WindowSystem, InputSystem, RenderSystem)
- Platform creates Application, calls lifecycle methods
- Systems register with Application, follow lifecycle protocol

### Threading Model

- **Application**: Single-threaded lifecycle (initialize/update/pause/resume/shutdown)
- **EventBus**: Thread-safe (reader-writer locks for listeners, lock-free queues for events)
- **EventEmitter types**:
  - SPSCEmitter: Single-producer, single-consumer (ReaderWriterQueue)
  - SPMCEmitter: Single-producer, multi-consumer (ConcurrentQueue)
  - MPMCEmitter: Multi-producer, multi-consumer (ConcurrentQueue)
- **Platform services**: Thread-compatible (not thread-safe, caller synchronizes)

### Lifetime & Ownership

- **Application**: Created by runApp, owns engine systems, destroyed after shutdown()
- **Platform**: Unique ownership, created in runApp, owns Application pointer
- **EventBus**: Shared ownership via shared_ptr (Subscription holds weak reference via shared_from_this)
- **Subscriptions**: Subscription holds shared_ptr<EventBus>, EventReceiver owns Subscriptions
- **Singletons**: Logger/Tracer/CommandLineParser initialized in runApp, shutdown in reverse order

### Platform Constraints

- Platform-specific implementations in saturn/platform/ (Win32, POSIX, AGDK, Emscripten, GLFW)
- Entry point differences:
  - Windows: WinMain (Win32), main (GLFW)
  - Linux: main (GLFW)
  - Android: android_main (AGDK, no SATURN_ENTRY_POINT)
  - Web: main (Emscripten GLFW)

---

## Modification Rules

### Safe to Change

- Add new ApplicationState values (extend state machine)
- Add new EngineSystemType entries (scene already added)
- Extend System base class with query methods (getState(), isInitialized())
- Add new event types (define struct, use EventBus::subscribe<Event>)
- Add new singleton services (follow Logger/Tracer pattern)
- Improve error messages in RefResult returns

### Requires Coordination

- Changing Application lifecycle affects all engine systems (window, input, graphics, scene)
- Modifying System interface breaks all EngineSystem/ClientSystem implementations
- Altering EventBus API affects all event consumers across codebase
- Changing entry_point.hpp macro affects testbed, editor, platform-specific builds
- Modifying Platform interface requires updating Win32/POSIX/AGDK/Emscripten/GLFW implementations

### Almost Never Change

- **Application state machine** — removing states breaks lifecycle protocol (add states only)
- **System lifecycle interface** — initialize/update/pause/resume/shutdown are load-bearing
- **SATURN_ENTRY_POINT signature** — platform builds depend on exact macro expansion
- **EventBus thread-safety guarantees** — removing locks breaks concurrent event emission
- **Singleton initialization order** — Logger before Tracer, CommandLineParser before Application

---

## Common Pitfalls

### Footguns

- ⚠️ **Calling update() before initialize()**: State machine violation, returns Err (check isInitialized())
- ⚠️ **Forgetting to disconnect Subscriptions**: Memory leak if EventBus outlives subscribers (use EventReceiver RAII)
- ⚠️ **EventBus use-after-free**: Subscription holds shared_ptr, but listener lambda may capture raw pointers
- ⚠️ **Multiple SATURN_ENTRY_POINT**: Linker error (multiple main definitions)
- ⚠️ **Platform::create() returns Result**: MUST check isErr() before dereferencing (Railway-Oriented)
- ⚠️ **EventBus::emitImmediate() blocking**: Synchronous event dispatch holds reader lock (use emitQueued for async)
- ⚠️ **Singleton access before initialize()**: Logger::getInstance() before Logger::initialize() returns nullptr

### Performance Traps

- 🐌 **emitImmediate() in hot paths**: Blocks on listener lock, snapshots callbacks (use emitQueued)
- 🐌 **Deep listener chains**: emitImmediate() calls listeners synchronously (stack overflow risk)
- 🐌 **EventBus::dispatchQueued() in update()**: Processes ALL queued events (may spike frame time)
- 🐌 **Large event structs**: Events copied into lock-free queues (keep events small, use pointers if needed)

### Historical Mistakes (Do NOT repeat)

- **Attempting to make Application movable**: Broke Pimpl pattern, removed move constructor
- **Adding exceptions to lifecycle methods**: Switched to Result<T, E> for Railway-Oriented Programming
- **Global EventBus singleton**: Switched to shared_ptr for testability and multi-app support

---

## How Claude Should Help

### Expected Tasks

- Add new EngineSystemType entries and system integration (e.g., SceneSystem)
- Extend Application with new lifecycle hooks (onResize, onFocus, etc.)
- Implement new event types (WindowResizeEvent, InputEvent, etc.)
- Add platform-specific SystemInfo queries (GPU info, memory, etc.)
- Write tests for Application lifecycle state machine
- Improve Logger sinks (file logging, network logging)
- Add Tracer instrumentation for profiling

### Conservative Approach Required

- **Changing Application state machine**: Verify all engine systems handle new states
- **Modifying System interface**: Check window/, input/, graphics/, scene/ implementations
- **Altering EventBus thread-safety**: Requires careful lock analysis (deadlock risk)
- **Breaking Platform::create() signature**: Affects all platform implementations (5 platforms)
- **Removing lifecycle states**: Breaks existing applications (deprecate instead)

### Before Making Changes

- [ ] Verify state machine transitions remain valid (initialize → resume → update loop → pause/shutdown)
- [ ] Check all EngineSystem implementations compile (WindowSystem, InputSystem, RenderSystem)
- [ ] Run core tests (no dedicated test file yet - add tests first)
- [ ] Test on multiple platforms (Win32, POSIX, Emscripten if touching Platform)
- [ ] Verify EventBus changes don't introduce data races (use ThreadSanitizer)
- [ ] Confirm Railway-Oriented error handling preserved (no exceptions thrown)

---

## Quick Reference

### Files

**Public API:**

- `include/saturn/core/application.hpp` — Application base class
- `include/saturn/core/system.hpp` — System, EngineSystem, ClientSystem, lifecycle states
- `include/saturn/core/platform.hpp` — Platform abstraction
- `include/saturn/core/events.hpp` — EventBus, EventEmitter, EventReceiver, Subscription
- `include/saturn/entry_point.hpp` — SATURN_ENTRY_POINT macro, runApp template
- `include/saturn/core/timer.hpp` — Timer for delta time
- `include/saturn/core/sys_console.hpp` — SystemConsole for terminal I/O
- `include/saturn/core/sys_ui.hpp` — SystemUI for native dialogs
- `include/saturn/core/sys_info.hpp` — SystemInfo for platform queries
- `include/saturn/core/cmd_line_parser.hpp` — CommandLineParser singleton
- `include/saturn/tools/logger.hpp` — Logger singleton
- `include/saturn/tools/tracer.hpp` — Tracer singleton

**Internal:**

- `src/core/application.cpp` — Application::Impl implementation
- `src/core/platform.cpp` — Platform factory
- `src/core/sys_console.cpp` — SystemConsole implementation
- `src/core/sys_ui.cpp` — SystemUI implementation
- `src/core/sys_info.cpp` — SystemInfo implementation
- `src/core/timer.cpp` — Timer implementation
- `src/core/cmd_line_parser.cpp` — CommandLineParser implementation
- `src/tools/logger.cpp` — Logger implementation
- `src/tools/tracer.cpp` — Tracer implementation

**Tests:**

- None currently (tests needed for state machine, EventBus)

### Key Functions/Methods

- `Application::initialize()` → RefResult<Application, string> — Transitions uninitialized → initialized
- `Application::update()` → RefResult<Application, string> — Called each frame
- `Application::pause()` → Transitions resumed → paused
- `Application::resume()` → Transitions paused/initialized → resumed
- `Application::shutdown()` → Transitions any state → shutdown
- `EventBus::subscribe<Event>(fn)` → Subscription — Register listener
- `EventBus::emitImmediate<Event>(args...)` → Synchronous dispatch
- `EventEmitter::emitQueued<Event>(args...)` → Async dispatch (lock-free)
- `EventBus::dispatchQueued()` → Process all queued events (call in update loop)
- `SATURN_ENTRY_POINT(AppType)` — Expands to platform-specific main()

### Build Flags

- `SATURN_PLATFORM_ANDROID` — Disables SATURN_ENTRY_POINT macro (android_main used instead)

---

## Status Notes

**Stable with active development** — Core lifecycle is production-ready. EventBus is new (in events.hpp, currently unstaged). Scene system integration pending.
