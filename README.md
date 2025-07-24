# 🎮 Mosaic Game Engine

**Mosaic** is a modern, cross-platform game engine written in **C++23**, built from the ground up as both a learning journey and a foundation for something greater.

Like all of my projects, Mosaic is first and foremost an exploration: a deep dive into systems programming, computer graphics, and game architecture. But it's not a toy. The engine is designed with the long-term goal of becoming a stable, professional-grade platform for interactive experiences — games, simulations, creative tools, and beyond.

Unlike many hobby engines that wrap existing libraries, Mosaic takes a bottom-up approach. I'm building its **core systems from scratch** — from a cache-friendly **Entity-Component-System (ECS)** and **layered input system**, to a **Vulkan/WebGPU-based renderer**, an **async-aware I/O layer**, and eventually a full **editor and asset pipeline**. It's about understanding, not just using.

Mosaic is still under heavy development — but the foundation is solid, and contributions or discussions are always welcome.

For more detailed documentation, guides, and API references, visit the [Mosaic Docs](https://williamkaroldicioccio.github.io/mosaic/).

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

Thanks for stopping by! 🌟

_— Di Cioccio William Karol_
