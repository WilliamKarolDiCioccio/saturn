## 🐞 Bug Report – Saturn Game Engine

### Pre-Issue Checklist ✅

Before opening this issue, please make sure you've:

- [ ] Searched existing [issues](../../issues) to avoid duplicates 🔍
- [ ] Pulled the latest changes from `main` and verified the bug still occurs 🧱
- [ ] Built the engine in **Release** and/or **Debug** mode to confirm it's not configuration-specific ⚙️
- [ ] Checked if the issue is **platform-specific** (Windows, Linux, Web, Android) 🧩

---

### 📋 Bug Summary

_A clear and concise description of the issue._

**Example:** The Vulkan renderer crashes on Intel iGPUs when resizing the window in fullscreen mode.

---

### 🔁 Steps to Reproduce

_Provide detailed steps to consistently reproduce the bug._

**Example:**

1. Run the engine with the Vulkan backend on a laptop with Intel graphics.
2. Set the window to fullscreen via `Engine::setFullscreen(true)`.
3. Resize the window or switch resolution.
4. Observe the crash and check logs.

---

### ✅ Expected Behavior

_What should have happened?_

**Example:** The engine should gracefully resize the framebuffer without crashing, even on integrated GPUs.

---

### ❌ Actual Behavior

_What actually happened? Include logs if available._

**Example:** Crash with validation error: `vkCmdSetViewport: viewport dimensions exceed framebuffer size`.

---

### 🧪 Platform Info

| Detail       | Info                            |
| ------------ | ------------------------------- |
| OS           | Windows 11 / Ubuntu 22.04 / Web |
| Build Config | Debug / Release                 |
| Graphics API | Vulkan / WebGPU                 |
| GPU Vendor   | NVIDIA / AMD / Intel            |
| Compiler     | MSVC / Clang / GCC              |

---

### 📸 Screenshots or Logs (Optional)

_Attach screenshots, videos, or error logs to help us understand the issue._

---

### 📂 Additional Context

_Add anything else relevant: hardware quirks, custom CMake flags, whether you're using emscripten, etc._

---

Thanks for helping improve Saturn! 💚
