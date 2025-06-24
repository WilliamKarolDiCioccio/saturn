# Coding Style and Naming Conventions

## Overview

This document outlines the coding style and naming conventions adopted in this project. The guidelines are primarily based on the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html), with specific adaptations to suit our development practices. Adhering to these conventions ensures code consistency, readability, and maintainability across the codebase.

## Code Formatting

We use **Clang Format** as the standard tool for code formatting. For specific formatting rules, refer to the configuration files:

- [.clang-format](./.clang-format)
- [.clang-tidy](./.clang-tidy)

We recommend using the **Clang Power Tools** extension if working in Visual Studio to ensure compliance with formatting standards.

## Integration with External Code

When incorporating third-party libraries, a balance must be maintained between adhering to internal coding standards and accommodating external APIs. Through layered abstraction, we isolate inconsistencies and maintain overall coherence in our codebase.

## Build System

Our build system is based on **CMake**, chosen for its flexibility, scalability, and wide support across platforms and IDEs. We use **VCPKG** as our cross-platform package manager to manage external dependencies. When a dependency is not available through VCPKG, it is added to the project as a Git submodule under the `vendor` directory located at the root of the repository. This ensures that all required libraries are version-controlled and easily accessible during the build process.

## Testing

We use **Google Test (GTest)** to implement both unit tests and integration tests. Testing is a critical part of the development lifecycle and helps us ensure the stability and correctness of the engine.

### Unit Tests

Unit tests are written for isolated components with well-defined inputs and outputs. Examples of what should be tested:

- **Math utilities**: Vector and matrix operations, quaternion conversions, interpolation functions.
- **Core engine systems**: Resource loading logic, scene graph hierarchy traversal, time step calculations.
- **Serialization**: Config file parsers, custom binary formats.

### Integration Tests

Integration tests cover interactions between components to ensure systems work together correctly. Examples include:

- Asset pipeline end-to-end conversion and import tests.
- Render pipeline setup and teardown for different hardware backends.
- ECS system behavior during entity/component creation and destruction.

### What Not to Test

Avoid testing:

- **Rendering output pixels**: These are often hardware- and driver-dependent and prone to false positives.
- **Third-party code**: Assume third-party libraries are well-tested and focus only on your integration logic.
- **Non-deterministic systems**: Avoid trying to test systems that rely on user input, random number generators, or real-time clocks unless the randomness is seeded and predictable.

A well-structured testing suite saves time during regression and improves confidence in changes across subsystems., chosen for its flexibility, scalability, and wide support across platforms and IDEs. We use **VCPKG** as our cross-platform package manager to manage external dependencies. When a dependency is not available through VCPKG, it is added to the project as a Git submodule under the `vendor` directory located at the root of the repository. This ensures that all required libraries are version-controlled and easily accessible during the build process.

## Class Structure

When defining classes with a large number of fields and methods, separate related sections using comments for improved readability.

```cpp
class ExampleClass {
 public:
  // Constructors and destructors
  ExampleClass();
  ~ExampleClass();

  // Public methods
  void initialize();
  void update();

 private:
  // Member variables
  int m_counter;
  float m_speed;
};
```

## Naming Conventions

### Functions

- Function names use **camelCase**.
- Function parameters use **camelCase** with an **underscore prefix (`_`)** to distinguish them from local variables.

```cpp
void processData(int _inputData);
```

### Class Fields

- Instance fields: **camelCase** with an **`m_` prefix**.
- Static fields: **camelCase** with an **`s_` prefix**.

```cpp
class MyClass {
 private:
  int m_id;
  static int s_instanceCount;
};
```

### Global Variables

- Use **camelCase** with a **`g_` prefix**.

```cpp
int g_applicationState;
```

### Class Names

- Use **PascalCase** without prefixes.

```cpp
class NetworkManager {};
```

### File Names

- Use **snake_case** to describe the file’s primary content, typically matching the main class name.

```bash
network_manager.cpp
```

### Macros and Constants

- Macros: Use **ALL_CAPS** with underscores. Restrict macro usage to source files to avoid polluting the global namespace.
- Constants: Use **camelCase** with a **`c_` prefix** to denote compile-time constants.

```cpp
#define MAX_BUFFER_SIZE 1024
const int c_defaultTimeout = 30;
```

### Enums

- Enum values use **camelCase** with underscores for readability and to avoid clashes with macros.

```cpp
enum class RenderMode {
  wireframe,
  shaded,
  textured,
};
```

## Documentation Conventions

We use **Doxygen** as the standard documentation generator for the engine. All documentation should strive to be **clear, concise, and helpful to both new and experienced contributors**.

### What to Document

- **All non-trivial public and internal APIs** must be documented.
- Any function or class that has:

  - **Side effects**
  - **Performance implications**
  - **Complex input/output behavior**

- Document **subsystems**, **core abstractions**, and **data flow** at a high level when relevant.
- Avoid redundant comments on trivial functions (e.g., `size()` on containers).

### Where to Document

- Use **Doxygen-style comments** directly in headers for functions, classes, and enums.
- For **complex implementation logic**, use in-source comments in `.cpp` files to explain:

  - Non-obvious control flow
  - Performance-critical code
  - Subtle algorithmic behavior

- If an entire file or subsystem needs explanation, place a Doxygen `\file` or `\brief` comment block at the top.
- Document major modules in standalone `.md` files under the `docs/` directory for higher-level guides.

### Style and Format

- Use **full sentences** with correct punctuation.
- Start function descriptions with a verb in **third-person singular** ("Returns", "Initializes", "Calculates").
- Mention:

  - Preconditions and postconditions
  - Ownership and lifetime expectations
  - Thread safety or concurrency constraints
  - Units of measurement (e.g., milliseconds, degrees, normalized)

```cpp
/**
 * Initializes the graphics subsystem and prepares rendering backends.
 *
 * Must be called before any rendering calls. This function allocates GPU memory and
 * sets up hardware-specific pipelines.
 *
 * @return True if initialization succeeded, false otherwise.
 */
bool Rendered::init();
```

### Code Modularization and Folder Structure

Our folder structure mirrors the modularization of the codebase: each namespace corresponds to a folder, except for `internal` and `detail`, which are reserved for implementation details. To prevent ambiguity when using `using namespace` directives or when referencing symbols within their own namespace, we prefer explicit and redundant naming (e.g., `mosaic::platform::win32::Win32Platform`). This approach, while more verbose, ensures clarity and avoids symbol collisions. As classes or systems grow in complexity, they should be moved into their own dedicated subfolders to further encapsulate related files and maintain a clean project hierarchy.

### Error Handling

We avoid using exceptions for flow control. Exceptions are only used in scenarios where crashing is acceptable or necessary (e.g., irrecoverable logic errors). In most cases, we prefer to use the `Result` class from the `pieces` template library to model recoverable errors. This encourages explicit handling and improves clarity when dealing with failure cases. If a function doesn't produce additional data on failure, a simple `bool` or `return code` may be used instead for simplicity or. For more complex error data without a success counterpart you might use `std::optional`. However, if the failure involves meaningful data for both error and sucess, even when already logged, a `Result` must still be returned so that the information can bubble up to a caller that may know how to respond appropriately.

### Class Lifecycle and API Design

To maintain predictable control over resource lifetimes and error propagation, we distinguish between object construction and initialization:

Constructors should be limited to establishing invariants and performing trivial setup. They must not perform operations that can fail. Any logic that may return an error should be delegated to an explicit `initialize()` method.

Initialization and shutdown logic should be clearly exposed through `initialize()` and `shutdown()` methods. These must return a Result or equivalent to enable proper error handling. Destruction must be idempotent and never fail.

Factory methods, typically named `create()`, are used to construct platform-specific instances returning a `std::unique_ptr` if initialization is guaranteed to succeed after construction.

For implementation-swappable classes, we adopt the PIMPL (Pointer to IMPLementation) idiom to decouple interface and implementation cleanly, enabling backend switching and faster build times.

For stateless or utility-like types that don’t require explicit lifetime management, static methods are preferred. These classes may wrap internal state behind private static singletons if needed.

This approach guarantees that initialization is always explicit and verifiable, reducing the risk of partially constructed or invalid objects and aligning with our result-based error model.

### Platform Specific Code and Logic

Platform-specific code does not always need to be isolated in a separate file or confined strictly to the platform layer. If the amount of platform-specific code is small and localized, it can be kept within the relevant source files, clearly marked with comments or preprocessor directives. For larger or more complex platform-dependent implementations, code should be organized into the dedicated platform layer subfolder or the appropriate subsystem folder to maintain clarity and separation of concerns. If you're only dealing with platform specific logic, meaning it doesn't directly interact with platform specific APIs but it is more of a behavior related difference, you can just compile it conditionally.

---

By following these conventions, we promote a unified and professional development experience. These guidelines aim to simplify onboarding, facilitate collaboration, and ensure long-term maintainability.

**Good Development,**  
_Di Cioccio William Karol_
