# Package: saturn/input

**Location:** `saturn/include/saturn/input/`, `saturn/src/input/`
**Type:** Part of saturn shared/static library
**Dependencies:** pieces (Result), window/ (Window), core/ (System), nlohmann-json (key mapping serialization)

---

## Purpose & Responsibility

### Owns

- Three-layer input architecture (InputSource → InputContext → Action)
- Platform-specific input sources (MouseInputSource, KeyboardInputSource, TextInputSource)
- Input context per window (state caching, virtual key mapping)
- High-level action system (named actions with lambda predicates)
- Time-based action detection (press, release, hold, double_press)
- Virtual key/button mapping (rebindable controls)
- JSON serialization of key mappings (load/save from file)
- Input state caching (current poll vs previous poll)

### Does NOT Own

- Window creation (window/ package)
- Platform event loop (window/ package handles GLFW polling)
- Gamepad/controller input (future feature)
- Touch input (future feature)

---

## Key Abstractions & Invariants

### Core Types

- **`InputSystem`** (`input_system.hpp:37`) — EngineSystem, manages contexts per window, singleton
- **`InputContext`** (`input_context.hpp:38`) — Per-window input state, owns sources, manages actions, Pimpl
- **`InputSource`** (`sources/input_source.hpp`) — Base class for platform-specific input
- **`MouseInputSource`** (`sources/mouse_input_source.hpp`) — Mouse position, buttons, wheel
- **`KeyboardInputSource`** (`sources/keyboard_input_source.hpp`) — Keyboard key states
- **`UnifiedTextInputSource`** (`sources/unified_text_input_source.hpp`) — Unified text input + IME composition (UTF-8/UTF-32)
- **`Action`** (`action.hpp:25`) — Named action with trigger predicate: `function<bool(InputContext*)>`
- **`KeyboardKey`** (`mappings.hpp`) — Enum of keyboard keys (virtual key codes)
- **`MouseButton`** (`mappings.hpp`) — Enum of mouse buttons (left, right, middle, etc.)

### Invariants (NEVER violate)

1. **One context per window**: InputSystem MUST maintain 1:1 mapping Window* → InputContext*
2. **Source ownership**: InputContext owns InputSources (unique_ptr), sources destroyed with context
3. **Action trigger purity**: Action::trigger MUST NOT modify InputContext (read-only predicate)
4. **Virtual key uniqueness**: Virtual key names MUST be unique within context (map<string, KeyboardKey>)
5. **State caching**: InputContext MUST cache previous poll state for edge detection (press vs hold)
6. **Update order**: InputSystem::update() MUST be called AFTER window event polling (GLFW pollEvents)
7. **JSON serialization**: Virtual key mappings MUST serialize to JSON (nlohmann-json)
8. **Platform-specific sources**: GLFW sources for desktop/web, AGDK sources for Android (mutually exclusive)
9. **Action registration idempotency**: Registering action with duplicate name MUST replace previous action
10. **Trigger evaluation**: isActionTriggered() MUST evaluate trigger lambda once per query (cached until next update)

### Architectural Patterns

- **Three-layer architecture**: InputSource (raw input) → InputContext (state management) → Action (high-level)
- **Pimpl**: InputContext and InputSystem hide implementation details
- **Singleton**: InputSystem::g_instance for global access
- **Strategy pattern**: InputSource subclasses for platform-specific input (GLFW vs AGDK)
- **Predicate-based actions**: Action::trigger lambda returns bool(InputContext\*)
- **Virtual key mapping**: String names map to platform-specific key codes (rebindable)
- **State caching**: Current state + previous state enable edge detection (press = down && !wasDown)
- **Unified text input**: UnifiedTextInputSource handles both regular text (WM*CHAR) and IME composition (WM_IME*\*) in single class
  - Filters WM_CHAR during active IME composition to prevent duplicate events
  - Emits both TextInputEvent (committed text) and IMEEvent (composition lifecycle)
  - Currently Win32-only; other platforms not yet implemented

---

## Architectural Constraints

### Dependency Rules

**Allowed:**

- pieces (Result)
- window/ (Window for context association)
- core/ (System lifecycle, Logger)
- nlohmann-json (virtual key mapping serialization)
- Platform input backends (GLFW, AGDK)

**Forbidden:**

- ❌ graphics/ — Input does NOT depend on rendering
- ❌ ecs/ — Input does NOT depend on ECS (user code bridges input → ECS)
- ❌ Direct platform input — Use InputSource abstraction (not raw GLFW/AGDK)

### Layering

- window/ creates Window → input/ creates InputContext for Window
- Application owns InputSystem (EngineSystem)
- InputContext wraps platform-specific InputSources

### Threading Model

- **InputSystem**: Single-threaded (called from Application::update())
- **InputContext**: Single-threaded (not thread-safe)
- **InputSources**: Single-threaded (platform input APIs not thread-safe)
- **Actions**: Trigger lambdas MUST be thread-safe if capturing external state

### Lifetime & Ownership

- **InputSystem**: Owned by Application, singleton g_instance
- **InputContext**: Owned by InputSystem, mapped by Window\*
- **InputSources**: Owned by InputContext via unique_ptr (addMouseSource(), etc.)
- **Actions**: Owned by InputContext, stored in vector<Action>

### Platform Constraints

- **GLFW**: Desktop (Windows, Linux, macOS), Web (Emscripten)
- **AGDK**: Android (native activity)
- **Source selection**: GLFW sources for GLFW windows, AGDK sources for AGDK windows (mutually exclusive)

---

## Modification Rules

### Safe to Change

- Add new InputSource types (GamepadInputSource, TouchInputSource)
- Extend Action with priority levels (high-priority actions check first)
- Add action modifiers (shift+key, ctrl+alt+key combinations)
- Improve virtual key mapping (load from multiple JSON files, merge mappings)
- Add input recording/playback (for testing, demos)
- Extend time-based detection (long_press, tap, swipe gestures)

### Requires Coordination

- Changing Action::trigger signature affects all user-defined actions
- Modifying KeyboardKey enum breaks JSON serialized mappings
- Altering InputContext API affects user code querying input
- Adding platform-specific sources requires CMake conditional compilation

### Almost Never Change

- **Three-layer architecture** — InputSource → InputContext → Action is core design
- **Predicate-based actions** — Lambda trigger pattern enables flexible action definitions
- **One context per window** — Multi-context per window breaks state isolation
- **Virtual key mapping** — Removing breaks rebindable controls feature

---

## Common Pitfalls

### Footguns

- ⚠️ **Calling update() before window poll**: State stale (call after GLFW pollEvents())
- ⚠️ **Action trigger modifies context**: Violates predicate purity (read-only!)
- ⚠️ **Not caching isActionTriggered() result**: Re-evaluates lambda multiple times per frame (expensive)
- ⚠️ **Virtual key name collision**: Registering "jump" twice overwrites previous mapping
- ⚠️ **Destroying InputContext while reading state**: Dangling pointer if user code holds InputContext\*
- ⚠️ **Mixing GLFW and AGDK sources**: Platform-exclusive (compile error or runtime crash)

### Performance Traps

- 🐌 **Complex action triggers**: Lambda evaluated every frame (keep triggers simple)
- 🐌 **Many actions per context**: O(n) scan for isActionTriggered() (cache triggered actions)
- 🐌 **JSON parsing every frame**: Load mappings at startup, not update loop
- 🐌 **Repeated virtual key translation**: translateKey() does map lookup (cache results)

### Historical Mistakes (Do NOT repeat)

- **Global input state**: Switched to per-window InputContext (multi-window support)
- **Hard-coded key bindings**: Added virtual key mapping for rebindable controls
- **Immediate mode input**: Added state caching for edge detection (press vs hold)

---

## How Claude Should Help

### Expected Tasks

- Add gamepad/controller support (GamepadInputSource, stick/trigger inputs)
- Implement touch input (TouchInputSource, multi-touch gestures)
- Add action priority system (high-priority actions cancel low-priority)
- Extend virtual key mapping (support key combinations: shift+A, ctrl+alt+delete)
- Write input recording/playback system (for testing, demos)
- Add IME support (Input Method Editor for Asian languages)
- Implement gesture recognition (swipe, pinch, rotate)
- Add input smoothing (mouse acceleration, dead zones)

### Conservative Approach Required

- **Changing Action::trigger signature**: Breaks all user-defined actions (coordinate carefully)
- **Modifying KeyboardKey enum**: Breaks serialized JSON mappings (add new keys only)
- **Altering state caching**: May break edge detection (press, release, hold)
- **Removing virtual key mapping**: Breaks rebindable controls (keep for accessibility)

### Before Making Changes

- [ ] Test on multiple platforms (GLFW: Windows/Linux/Web, AGDK: Android)
- [ ] Verify virtual key JSON serialization works (load/save roundtrip)
- [ ] Test action trigger edge cases (press, release, hold, double_press)
- [ ] Check multi-window input isolation (each context independent)
- [ ] Validate platform-specific sources compile (GLFW vs AGDK conditional)
- [ ] Test input lag (update() called after window poll, before render)

---

## Quick Reference

### Files

**Public API:**

- `include/saturn/input/input_system.hpp` — InputSystem, manages contexts
- `include/saturn/input/input_context.hpp` — InputContext, manages sources and actions
- `include/saturn/input/action.hpp` — Action struct
- `include/saturn/input/sources/input_source.hpp` — InputSource base class
- `include/saturn/input/sources/mouse_input_source.hpp` — MouseInputSource
- `include/saturn/input/sources/keyboard_input_source.hpp` — KeyboardInputSource
- `include/saturn/input/sources/unified_text_input_source.hpp` — UnifiedTextInputSource (text + IME)
- `include/saturn/input/mappings.hpp` — KeyboardKey, MouseButton enums
- `include/saturn/input/events.hpp` — Input event types
- `include/saturn/input/constants.hpp` — Input constants

**Internal:**

- `src/input/input_system.cpp` — InputSystem implementation
- `src/input/input_context.cpp` — InputContext implementation
- `src/input/sources/mouse_input_source.cpp` — MouseInputSource implementation
- `src/input/sources/keyboard_input_source.cpp` — KeyboardInputSource implementation
- `src/input/sources/unified_text_input_source.cpp` — UnifiedTextInputSource implementation

**Platform-Specific:**

- `src/platform/GLFW/glfw_mouse_input_source.cpp` — GLFW mouse source
- `src/platform/GLFW/glfw_keyboard_input_source.cpp` — GLFW keyboard source
- `src/platform/Win32/win32_unified_text_input_source.cpp` — Win32 unified text + IME source

**Tests:**

- None currently (tests needed for action triggers, virtual key mapping)

### Key Functions/Methods

- `InputSystem::registerWindow(Window*)` → Result<InputContext\*, string> — Create context for window
- `InputSystem::update()` → Update all contexts (call after window poll)
- `InputContext::registerActions(vector<Action>)` — Register named actions
- `InputContext::isActionTriggered(name, onlyCurrPoll=true)` → bool — Check if action fired
- `InputContext::loadVirtualKeysAndButtons(filePath)` — Load key mappings from JSON
- `InputContext::translateKey(name)` → KeyboardKey — Convert virtual name to platform key
- `InputContext::addMouseSource()` → Add mouse input source
- `InputContext::addKeyboardSource()` → Add keyboard input source

### Build Flags

- None (platform sources selected via CMake conditionals)

---

## Status Notes

**Stable** — Core three-layer architecture functional. Unified text input with full IME support on Win32. Gamepad/controller and touch input not implemented. Text input on GLFW/AGDK/Emscripten platforms not yet implemented.
