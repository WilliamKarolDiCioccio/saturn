# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## General Instructions

- In all interactions and commit messages, be extremely concise and sacrifice grammar for the sake of concision.
- At the end of each plan, me a list of unresolved questions to answer, if any. Make the questions extremely concise. Sacrifice grammar for the sake of concision.

## Agent Delegation Strategy

**DEFAULT: Delegate all engine-related tasks to `engine-architect` (LEAD agent). It coordinates specialist team.**

### Team Structure

**LEAD Agent (default for all engine work):**

- `engine-architect` — Conservative C++23 systems engineer. Coordinates specialist team. Authority over planning, implementation, cross-package refactors.

**Specialist Team (coordinated by LEAD):**

- `performance-specialist` — Performance analysis, profiling, bottleneck identification (analysis only, NO code modification)
- `test-ci-agent` — Unit tests, integration tests, CI/GTest infrastructure (tests YES, production code NO)
- `doc-context-manager` — CLAUDE.md files, documentation, comments, Astro website (docs YES, runtime code NO)
- `dart-flutter-tooling-agent` — Flutter tooling, Dart packages, native integrations (tooling YES, engine internals NO)

### Default Delegation Rules

**For any task involving:**

- C++ engine code (saturn/, pieces/, codex/)
- Architecture planning/implementation
- Core systems (ECS, rendering, exec, input, platform)
- Cross-package changes

**→ Delegate to `engine-architect` immediately. Let it coordinate specialists.**

### Direct Specialist Delegation (bypass LEAD)

Only delegate directly to specialists when:

- **Standalone documentation work** → `doc-context-manager`
- **Standalone test work (no engine changes)** → `test-ci-agent`
- **Performance analysis only (no implementation)** → `performance-specialist`
- **Flutter/Dart tooling only** → `dart-flutter-tooling-agent`

**If task touches engine code + docs/tests/perf → Use `engine-architect` to coordinate.**

## Gemini CLI Integration

**See `GEMINI.md` for full rules.** Gemini is a support agent, not a peer.

### Mandatory Large File Rule

Before reading ANY file, check line count:

```bash
python .claude/skills/invoke-gemini/scripts/check_line_count.py <file>
```

- `>1000 lines` → Gemini MUST analyze first; Claude MUST NOT read directly
- `<=1000 lines` → Claude proceeds normally

### Gemini Use Cases

**Use Gemini for:**

- Large file structural analysis (>1000 lines)
- Flutter UI / Dart binding boilerplate
- Repetitive code generation (serialization, adapters)
- Platform scaffolding

**Never use Gemini for:**

- ECS, renderer, threading, memory management
- Architectural decisions
- Any deep reasoning task

### Workflow

1. Claude defines spec/constraints
2. Gemini generates bulk output
3. Claude reviews and integrates

## Hierarchical Context System

**This repository uses package-level CLAUDE.md files for deep, localized context.**

### Rules for Claude

**ALWAYS:**

1. When working in a subdirectory with a CLAUDE.md, load and consult that file FIRST
2. Treat package-level files as **authoritative** for local decisions (invariants, constraints, modification rules)
3. Use package-level "Owns/Does NOT Own" sections to determine responsibility boundaries
4. Check package-level "Invariants" before making ANY changes to that package
5. Follow package-level "Modification Rules" for scope determination

**Precedence:**

- Package-level CLAUDE.md > Root CLAUDE.md (for local decisions)
- Root CLAUDE.md > Package-level (for global patterns, build system, cross-cutting concerns)
- In conflicts: Package-level invariants override general guidance

**Fallback to Root:**

- Build system (CMake, vcpkg, compilation)
- Cross-package architecture
- Platform-specific build notes
- Global coding patterns (Result<T,E>, logging, testing)
- Development workflow

### Package-Level CLAUDE.md Locations

**Top-Level Modules:**

- `pieces/CLAUDE.md` — Header-only utility library (zero dependencies)
- `codex/CLAUDE.md` — C++ parser tool
- `docsgen/CLAUDE.md` — Documentation generator

**Engine Subsystems (`saturn/`):**

- `saturn/core/CLAUDE.md` — Application lifecycle, system registry
- `saturn/ecs/CLAUDE.md` — Entity-Component-System (archetypal, type-erased)
- `saturn/exec/CLAUDE.md` — Multi-threaded execution (ThreadPool, TaskFuture)
- `saturn/graphics/CLAUDE.md` — Rendering (Vulkan/WebGPU backends)
- `saturn/input/CLAUDE.md` — Input system (three-layer architecture)
- `saturn/platform/CLAUDE.md` — Platform abstraction (Win32/POSIX/AGDK/Emscripten/GLFW)
- `saturn/window/CLAUDE.md` — Window management
- `saturn/scene/CLAUDE.md` — Scene graph (in development - stub files)

### When to Consult Package Files

**Before modifying code:**

- Check if file is in a package with CLAUDE.md
- Read "Invariants (NEVER violate)" section
- Review "Modification Rules" → "Safe to Change" vs "Requires Coordination"

**When adding features:**

- Check "Owns/Does NOT Own" to verify responsibility
- Review "Architectural Constraints" → "Dependency Rules"
- Follow "How Claude Should Help" guidance

**When debugging:**

- Consult "Common Pitfalls" section for known footguns
- Check "Performance Traps" if issue is performance-related

**When unsure:**

- Package-level "Almost Never Change" lists load-bearing code
- "Expected Tasks" clarifies what Claude should assist with
- "Conservative Approach Required" highlights danger zones

## Project Overview

Saturn is a modern, cross-platform game engine written in C++23. It's built from the ground up with handcrafted core systems including a cache-friendly ECS, layered input system, Vulkan/WebGPU renderer, and async-aware execution layer.

## Build System

### Prerequisites

- **CMake**: 3.24 or higher
- **vcpkg**: Used for dependency management (managed via vcpkg.json)
- **C++23 compiler**: MSVC, GCC, or Clang
- **Vulkan SDK**: Required for non-Emscripten builds

### Building the Project

**Windows (PowerShell):**

```powershell
cmake --preset=windows-multi
cmake --build --preset=windows-multi-debug
```

**Linux:**

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

**Emscripten (WebAssembly):**

```bash
emcmake cmake -B build-web -S .
cmake --build build-web
```

### Build Configurations

The project supports multiple build configurations (see `cmake/ConfigureForBuildType.cmake`):

- **Debug**: No optimization, full debug symbols, assertions enabled
- **Dev**: Optimized with debug symbols, link-time optimization
- **Release**: Full optimization, hardened security, static analysis (MSVC)
- **Sanitizers**: Address/undefined/thread sanitizers enabled

### Running Tests

**All tests:**

```bash
ctest --test-dir build
```

**Specific test executable:**

```bash
./build/saturn/tests/saturn_tests
```

**Individual test suite:**

```bash
./build/saturn/tests/saturn_tests --gtest_filter=ECS*
```

### Running Benchmarks

```bash
./build/saturn/bench/saturn_bench
```

### Building Documentation

The `docsgen` tool generates API documentation:

```bash
./build/docsgen/docsgen --input ./saturn/include --output ./docs
```

### Shader Compilation

Shaders are compiled automatically post-build via:

- Windows: `scripts/compile_shaders.ps1`
- Linux/Mac: `scripts/compile_shaders.sh`

Manual compilation is rarely needed as CMake handles this.

## Architecture

### Module Structure

The codebase is organized into four main modules:

1. **pieces** (`pieces/`)
   - Header-only utility library with zero dependencies
   - Core building blocks: type-erased containers, memory allocators, result types, string utilities
   - Key components:
     - Containers: `SparseSet`, `TypelessVector`, `CircularBuffer`, `ConstexprMap`, `Bitset`
     - Memory: `PoolAllocator`, `ContiguousAllocator`, `ProxyAllocator`
     - Error handling: `Result<T, E>` for Railway-Oriented Programming (Rust/F#-inspired)
     - Async: `Task<T>` coroutine utilities for C++20 async programming
     - Performance: SIMD intrinsics wrapper, cache-line alignment utilities
     - Strings: UTF-8 support and conversion utilities

2. **saturn** (`saturn/`)
   - Main game engine library (shared library on desktop, static on mobile/web)
   - Organized by system:
     - `core/`: Application lifecycle, platform abstraction, system registry
     - `ecs/`: Entity-Component-System with archetype-based storage
     - `exec/`: Thread pool, task scheduler, async task futures
     - `graphics/`: Render system with Vulkan and WebGPU backends
     - `input/`: Context-aware input system with stateful bindings
     - `window/`: Cross-platform window management
     - `scene/`: Scene graph and entity hierarchy (**in development - stub files**)
   - Platform implementations in `src/platform/`:
     - `Win32/`: Windows-specific code
     - `POSIX/`: Linux-specific code
     - `AGDK/`: Android Game Development Kit
     - `Emscripten/`: WebAssembly/browser
     - `GLFW/`: Cross-platform windowing (desktop only)

3. **codex** (`codex/`)
   - C++ source code parser and documentation extractor
   - Uses tree-sitter for parsing
   - Generates structured data for documentation tooling

4. **docsgen** (`docsgen/`)
   - Documentation generation tool
   - Consumes codex output to generate API docs

### Cross-Cutting Systems

**Entity-Component-System (ECS):**

- **Architecture**: Purely archetypal, type-unaware implementation
- **Archetype-based storage**: Entities with identical component signatures stored contiguously
- **Type-erased components**: Runtime component registration using `ComponentSignature` (256-bit bitset)
- **Cache-friendly iteration**: Components packed tightly in archetype tables for data locality
- **Entity IDs**: Generational indices to detect use-after-free
- Core types: `EntityRegistry`, `Archetype`, `ComponentRegistry`, `TypelessSparseSet`
- Key operations: `createEntity<Ts...>()`, `addComponents<Ts...>()`, `viewSubset<Ts...>()`, `forEach()`
- Storage pattern: Each archetype row contains `[EntityMeta | Component1 | Component2 | ... | ComponentN]`

**Multi-threaded Execution:**

- `ThreadPool` for work-stealing task execution (one worker per logical CPU core)
- `TaskFuture<T>` for async/await-style programming with **cancellation support**
- `TaskPromise<T>` for the promise half of the future/promise pair
- `SharedState<T>` with spin-then-wait optimization for low latency
- `TaskGraph` and `TaskScheduler` for dependency-based execution (DAG-based)
- Worker affinity control for CPU pinning
- Multiple queue strategies: global queue + per-worker queues
- Reduced heap allocations compared to `std::future`

**Rendering Architecture:**

- Backend abstraction: `RenderContext` and `RenderSystem` base classes
- Two implementations:
  - Vulkan backend (Windows/Linux/Android) in `graphics/Vulkan/`
  - WebGPU backend (web via Emscripten + desktop via Dawn) in `graphics/WebGPU/`
- **Frame lifecycle**: `beginFrame()` → `updateResources()` → `drawScene()` → `endFrame()`
- Platform-specific surface creation handled in window backends
- Uses volk for Vulkan function loading (no static linking)
- VMA (Vulkan Memory Allocator) for efficient GPU memory management
- Vulkan organized in subdirectories: `context/`, `commands/`, `pipelines/`

**Input System:**

- **Three-layer architecture**:
  1. **Input Sources** (platform-specific): `MouseInputSource`, `KeyboardInputSource`, `TextInputSource`
  2. **Input Context** (window-specific): State caching, virtual key mapping, JSON serialization
  3. **Action System** (high-level): Named actions with lambda predicates, state detection
- Virtual key mapping for rebindable controls
- Time-based action detection: `press`, `release`, `hold`, `double_press`
- Duration tracking for hold actions
- Context-aware triggering with priority stacking
- Platform abstraction: GLFW (desktop/web), AGDK (Android)

**Platform Abstraction:**

- `Platform` base class with platform-specific implementations
- System info, console I/O, and UI dialogs abstracted
- Conditional compilation based on target platform

### Entry Point and Application Lifecycle

Applications use `SATURN_ENTRY_POINT(AppClass)` macro which provides platform-specific entry points (WinMain, main, android_main).

**Application Base Class:**

```cpp
class MyGame : public saturn::Application {
    void onInitialize() override { /* setup */ }
    void onUpdate() override { /* game loop */ }
    void onShutdown() override { /* cleanup */ }
};

SATURN_ENTRY_POINT(MyGame)
```

**System Lifecycle:**

- Systems have lifecycle states: `uninitialized` → `initialized` → `resumed` ⇄ `paused` → `shutdown`
- `System` base interface with virtual lifecycle methods
- `EngineSystem`: Engine-managed systems (window, input, render)
- `ClientSystem`: User-defined game systems

The `Application` class manages:

- System initialization/shutdown order
- Update loop with fixed/variable timestep
- Event dispatching
- Resource management

## Code Patterns and Conventions

### Error Handling (Railway-Oriented Programming)

Use `pieces::Result<T, Error>` for fallible operations with monadic composition:

```cpp
auto result = initialize()
    .andThen([](auto& sys) { return sys.configure(); })
    .andThen([](auto& sys) { return sys.start(); });

if (result.isErr()) {
    saturn::Logger::error("Failed: {}", result.error());
}
```

Also supports `RefResult<T, E>` for reference wrappers.

### Logging

Use the centralized logger (fmt-based):

```cpp
#include <saturn/tools/logger.hpp>
saturn::Logger::info("Message: {}", value);
saturn::Logger::error("Error: {}", error);
```

### Platform-Specific Code

Platform code should be isolated in:

- `saturn/src/platform/<Platform>/` for implementations
- Conditional compilation in CMakeLists.txt (not #ifdef in headers when possible)

### Testing

- Unit tests use Google Test framework
- Tests located in `saturn/tests/unit/`
- Mocks available in `saturn/tests/mocks/`
- Test naming: `<feature>_test.cpp`

### Benchmarks

- Use Google Benchmark framework
- Located in `saturn/bench/` and `pieces/bench/`

## Design Patterns Used

### Type Erasure

Type-erased containers enable runtime component registration in the ECS:

- `TypelessSparseSet`: Stores arbitrary component types without templates
- `TypelessVector`: Dense storage with byte-level manipulation
- Enables dynamic component loading from scripts/plugins

### Pimpl (Pointer to Implementation)

Reduces compilation dependencies and hides implementation details:

- Used in: `Application`, `Window`, `InputContext`, `ThreadPool`, `RenderContext`
- Pattern: Forward-declared `Impl` struct with opaque pointer

### Factory Pattern

Backend selection at runtime:

```cpp
auto renderSystem = RenderSystem::create(RendererAPIType::vulkan);
```

### CRTP (Curiously Recurring Template Pattern)

Static polymorphism for performance:

```cpp
template <typename T>
class PoolAllocator : public NonCopyable<PoolAllocator<T>> { };
```

## Platform-Specific Notes

### Windows

- Requires Win32 SDK for native window creation
- Uses GLFW for cross-platform window management
- Vulkan SDK must be installed

### Linux

- Requires X11 development libraries
- GLFW handles windowing
- May need to set `VCPKG_ROOT` environment variable

### Android

- Built as shared library loaded by Java/Kotlin app
- Uses AGDK (Android Game Development Kit) for native activity
- Android project in `testbed_android/`
- Vulkan is the only rendering backend (no WebGPU)

### Emscripten/Web

- Built as static library with .html output
- WebGPU is the only rendering backend (no Vulkan)
- GLFW is provided by Emscripten (not built from source)
- Special flags: `-pthread`, `-sUSE_PTHREADS=1`, `-sASYNCIFY`

## Dependencies

Key dependencies (managed via vcpkg.json):

- **Graphics**: Vulkan SDK, volk, vulkan-memory-allocator, glfw3, glm
- **Utilities**: fmt (formatting), nlohmann-json, utf8cpp
- **Testing**: gtest, benchmark
- **Parsing**: tree-sitter
- **CLI**: bfgroup-lyra
- **Concurrency**: readerwriterqueue, concurrentqueue

## Development Workflow

1. Make changes to source files
2. Build the project with appropriate configuration
3. Run tests to verify changes: `ctest --test-dir build`
4. For performance-sensitive code, run benchmarks
5. The `testbed` application is a sandbox for testing engine features

## Common Issues

### vcpkg Integration

If CMake can't find vcpkg packages, ensure `VCPKG_ROOT` is set or vcpkg toolchain file is specified:

```bash
cmake -B build -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
```

### Vulkan SDK

Ensure Vulkan SDK is installed and `VULKAN_SDK` environment variable is set.

### Shader Compilation Failures

Shaders require `glslc` (from Vulkan SDK) to be in PATH. Check shader compiler output in build logs.
