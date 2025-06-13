# 🎮 Mosaic Game Engine

**Mosaic** is a modern, cross-platform game engine written in **C++23**, built from the ground up as both a learning journey and a foundation for something greater.

Like all of my projects, Mosaic is first and foremost an exploration: a deep dive into systems programming, computer graphics, and game architecture. But it's not a toy. The engine is designed with the long-term goal of becoming a stable, professional-grade platform for interactive experiences — games, simulations, creative tools, and beyond.

Unlike many hobby engines that wrap existing libraries, Mosaic takes a bottom-up approach. I'm building its **core systems from scratch** — from a cache-friendly **Entity-Component-System (ECS)** and **layered input system**, to a **Vulkan/WebGPU-based renderer**, an **async-aware I/O layer**, and eventually a full **editor and asset pipeline**. It's about understanding, not just using.

Mosaic is still under heavy development — but the foundation is solid, and contributions or discussions are always welcome.

---

## ✨ Goals and Highlights

- 🧱 **Build it to understand it** — nearly every core system is handcrafted, from ECS to rendering.
- 🌍 **Cross-platform** — supports Windows, Linux, and the Web (via WebGPU).
- 🎨 **Modern graphics** — Vulkan backend, with a future-facing WebGPU renderer for browser targets.
- 🧠 **Layered architecture** — clean separation of engine, editor, runtime, and testbed.
- 🖱 **Input like Flutter/Unity** — rich input system with context-aware, stateful bindings.
- 🔄 **Sync + Async APIs** — inspired by Dart’s ergonomic approach to I/O.
- 🧪 **Stress-tested** — with unit tests and integration tests covering critical systems.
- 🛠 **CMake + vcpkg** — modern and maintainable build setup, with submodules where needed.

---

## 🛠 Getting Started

Mosaic uses CMake as its build system. Most dependencies are managed via vcpkg, while those unavailable through vcpkg — such as WebGPU, utility libraries, or components requiring special build profiles — are included as submodules under the vendor/ directory.

On Windows, we recommend using Visual Studio 2022 over VS Code for better CMake integration. The default compiler on Windows is MSVC, but Clang is also supported — and is the preferred compiler on Linux and Web targets (via Emscripten).

### 🌲 Environment Setup

> [!NOTE]
> Before building Mosaic, you'll need to set up a few tools and SDKs. The engine targets **Windows**, **Linux**, **Web** and **Android** but you can ignore requirements for the build configurations you're not interested in.

#### ✅ Prerequisites

- CMake 3.25 or later
- Git — with submodule support
- Python 3 — required for Emscripten and some build scripts
- A C++23-compliant compiler (MSVC ≥ 17.13 Preview 2, Clang ≥ 13, or GCC ≥ 14)

#### 📦 Install vcpkg

We use **vcpkg in manifest mode** to manage most dependencies.

1. Clone vcpkg:

   ```bash
   git clone https://github.com/microsoft/vcpkg.git
   ./vcpkg/bootstrap-vcpkg.sh  # or .\bootstrap-vcpkg.bat on Windows
   ```

2. No need to integrate it globally — Mosaic uses the local `vcpkg.json` manifest.

3. If needed, set the environment variable `VCPKG_ROOT` to point to your vcpkg directory.

#### 🌐 Setup for Web (Emscripten/WebGPU)

1. Install the **Emscripten SDK**:

   ```bash
   git clone https://github.com/emscripten-core/emsdk.git
   cd emsdk
   ./emsdk install latest
   ./emsdk activate latest
   source ./emsdk_env.sh  # or emsdk_env.bat on Windows
   ```

2. Ensure `emcc` is in your `PATH` before running CMake.

3. Confirm that `-pthread`, `-matomics`, and `-mbulk-memory` flags are supported — see build notes below if WebGPU issues arise.

> [!NOTE]
> We suggest debugging Emscripten builds directly inside the browser either using the built-in dev-tools or installing the WASM debugging plugin available on the [Chrome Web Store](https://chromewebstore.google.com/detail/cc++-devtools-support-dwa/pdcpmagijalfljmkmjngeonclgbbannb). The project is automatically configured to generate teh relevant debug information.

#### 🪟 Setup for Windows (Visual Studio 2022)

- Install **Visual Studio 2022** with the following components:

  - "Desktop development with C++"
  - "C++ CMake tools for Windows"
  - "MSVC v143" or later
  - "Windows 10 or 11 SDK"

#### 🐧 Setup for Linux

- Install **required packages**:

  ```bash
  sudo apt update
  sudo apt install build-essential cmake git ninja-build python3 python3-pip libx11-dev libxcursor-dev libxrandr-dev libxi-dev libgl1-mesa-dev libvulkan-dev
  ```

#### 📱 Setup for Android (Experimental)

> Android support is a work in progress. The current renderer uses Vulkan, so minimum API level 24 is required.

1. Install **Android Studio**.
2. Inside the SDK Manager, install:

   - Android NDK (r23 or newer)
   - CMake and LLDB
   - SDK tools for API level 24+

3. Set the environment variable `ANDROID_NDK_HOME` to point to your NDK installation.

### 🔧 Build Instructions

- Clone the repository and initialize submodules. `git clone --recurse-submodules https://github.com/WilliamKarolDiCioccio/mosaic`.

- Select a CMake configure preset, then a build preset (the vcpkg toolchain manages installation automatically).

- Build the project — and you're good to go!

> [!WARNING]
> When building for the Web, you might encounter issues related to threading support.
>
> Mosaic uses the -pthread flag in Emscripten to enable Web Workers, which helps retain compatibility with native threading code. Occasionally, you may need to manually patch the build system to support this.
>
> If the build fails with missing atomic features, add the following flag manually in the `emdawnwebgpu.port.py` file located at:
>
> `out/build/emscripten-${PROFILE}/\_deps/emdawnwebgpu-src/`
>
> Update the flags section as follows:
>
> ```py
> flags = ['-g', '-std=c++17', '-fno-exceptions']
> flags += ['-matomics', '-mbulk-memory'] # <-- Add this line
> flags += compute_library_compile_flags(settings)
> ```

This ensures compatibility with multi-threaded WebAssembly targets.

---

## 📄 License

Mosaic comes under the permissive MIT License to encourage contributions. See the [`LICENSE.md`](./LICENSE.md) file for more information.

---

Thanks for stopping by! 🌟

_— Di Cioccio William Karol_
