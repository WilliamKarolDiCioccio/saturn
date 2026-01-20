# Package: saturn/graphics

**Location:** `saturn/include/saturn/graphics/`, `saturn/src/graphics/`
**Type:** Part of saturn shared/static library
**Dependencies:** pieces (Result), window/ (Window), core/ (System), Vulkan SDK (volk, VMA), WebGPU (Dawn/Emscripten)

---

## Purpose & Responsibility

### Owns

- Rendering abstraction layer (RenderSystem, RenderContext)
- Backend implementations (Vulkan, WebGPU)
- Frame lifecycle (beginFrame → updateResources → drawScene → endFrame)
- Surface creation and swapchain management
- GPU memory allocation (VMA for Vulkan)
- Command buffer recording and submission
- Render pass management
- Pipeline and shader compilation
- Per-window render contexts (multi-window support)

### Does NOT Own

- Window creation (window/ package)
- Input handling (input/ package)
- Scene graph or entity management (scene/ and ecs/ packages)
- Asset loading (textures, models, shaders from disk)
- High-level rendering API (materials, meshes, lights - user code)

---

## Key Abstractions & Invariants

### Core Types

- **`RenderSystem`** (`render_system.hpp:26`) — EngineSystem base, owns contexts, factory pattern, singleton
- **`RendererAPIType`** (`render_system.hpp:19`) — Enum: web_gpu, vulkan, none
- **`RenderContext`** (`render_context.hpp:23`) — Per-window render target, frame lifecycle, Pimpl
- **`RenderContextSettings`** (`render_context.hpp:12`) — enableDebugLayers, backbufferCount

### Vulkan Backend Types (src/graphics/Vulkan/)

- **`VulkanInstance`** (`context/vulkan_instance.hpp`) — Vulkan instance, validation layers
- **`VulkanDevice`** (`context/vulkan_device.hpp`) — Physical/logical device, queue families
- **`VulkanSurface`** (`context/vulkan_surface.hpp`) — Window surface (Win32/Xlib/Wayland/Android)
- **`VulkanSwapchain`** (`vulkan_swapchain.hpp`) — Swapchain, image views, present mode
- **`VulkanAllocator`** (`vulkan_allocator.hpp`) — VMA wrapper for GPU memory
- **`VulkanCommandPool`** (`commands/vulkan_command_pool.hpp`) — Command buffer allocation
- **`VulkanCommandBuffer`** (`commands/vulkan_command_buffer.hpp`) — Command recording
- **`VulkanRenderPass`** (`commands/vulkan_render_pass.hpp`) — Render pass, attachments
- **`VulkanPipeline`** (`pipelines/vulkan_pipeline.hpp`) — Graphics pipeline state
- **`VulkanShaderModule`** (`pipelines/vulkan_shader_module.hpp`) — SPIR-V shader loading
- **`VulkanFramebuffers`** (`vulkan_framebuffers.hpp`) — Framebuffer objects

### WebGPU Backend Types (src/graphics/WebGPU/)

- **`WebGPUInstance`** (`webgpu_instance.hpp`) — WebGPU instance (Dawn or Emscripten)
- **`WebGPUDevice`** (`webgpu_device.hpp`) — Device, adapter, queue
- **`WebGPUSwapchain`** (`webgpu_swapchain.hpp`) — Swapchain, texture views
- **`WebGPUCommands`** (`webgpu_commands.hpp`) — Command encoder, render pass
- **`WebGPUPipeline`** (`webgpu_pipeline.hpp`) — Render pipeline

### Invariants (NEVER violate)

1. **Backend exclusivity**: ONLY one backend active at compile time (Vulkan OR WebGPU, never both)
2. **Platform constraints**: Vulkan on desktop/Android, WebGPU on web (+desktop via Dawn)
3. **One context per window**: RenderSystem MUST maintain 1:1 mapping Window* → RenderContext*
4. **Frame lifecycle order**: MUST call beginFrame() → updateResources() → drawScene() → endFrame() (strict ordering)
5. **Surface recreation on resize**: RenderContext MUST recreate swapchain on window resize (resizeFramebuffer())
6. **VMA ownership**: VulkanAllocator owns VmaAllocator, MUST NOT use VMA directly outside allocator
7. **Command buffer recording**: MUST begin command buffer before recording, end before submission
8. **Synchronization**: MUST use fences/semaphores for frame-in-flight synchronization (acquire → render → present)
9. **Validation layers**: enableDebugLayers MUST be false in release builds (performance)
10. **Backend initialization**: RenderSystem subclass (VulkanRenderSystem/WebGPURenderSystem) MUST initialize backend before creating contexts

### Architectural Patterns

- **Factory pattern**: RenderSystem::create(RendererAPIType) returns backend-specific subclass
- **Pimpl**: RenderSystem and RenderContext hide implementation details
- **Singleton**: RenderSystem::g_instance for global access
- **Frame lifecycle**: Explicit begin/end frame for double/triple buffering
- **Command recording**: Record commands into command buffers, submit batched
- **Resource ownership**: RenderContext owns swapchain, framebuffers, command pools

---

## Architectural Constraints

### Dependency Rules

**Allowed:**

- pieces (Result)
- window/ (Window for surface creation)
- core/ (System lifecycle, Logger)
- Vulkan SDK (volk for dynamic loading, VMA for memory allocation)
- WebGPU (Dawn for desktop, Emscripten GLFW WebGPU binding for web)
- glm (math library for shaders/transforms)

**Forbidden:**

- ❌ ecs/ — Rendering does NOT depend on ECS (scene/ may integrate both)
- ❌ scene/ — Graphics is lower-level than scene graph
- ❌ Direct VMA usage outside VulkanAllocator — Encapsulate in allocator
- ❌ Static Vulkan linking — Use volk for dynamic loading

### Layering

- window/ creates Window → graphics/ creates RenderContext for Window
- Application owns RenderSystem (EngineSystem)
- RenderContext wraps backend-specific rendering (Vulkan/WebGPU)

### Threading Model

- **RenderSystem**: Single-threaded (called from Application::update())
- **RenderContext**: Single-threaded frame lifecycle (beginFrame/endFrame not thread-safe)
- **Command buffers**: Can be recorded in parallel (Vulkan supports secondary command buffers)
- **VMA**: Thread-safe allocations (internal synchronization)

### Lifetime & Ownership

- **RenderSystem**: Owned by Application, singleton g_instance
- **RenderContext**: Owned by RenderSystem, mapped by Window\*
- **Swapchain**: Owned by RenderContext, recreated on resize
- **Framebuffers**: Owned by RenderContext (Vulkan) or transient (WebGPU)
- **Command pools/buffers**: Owned by RenderContext, per-frame-in-flight

### Platform Constraints

- **Vulkan**: Windows, Linux, Android (requires Vulkan SDK)
- **WebGPU**: Web (Emscripten), desktop (Dawn)
- **No Vulkan on web**: Emscripten builds MUST use WebGPU backend
- **No WebGPU on Android**: Android MUST use Vulkan backend
- **Surface creation**: Platform-specific (Win32, Xlib, Wayland, AGDK, Emscripten GLFW)

---

## Modification Rules

### Safe to Change

- Add new shader types (compute, tessellation, geometry)
- Extend RenderContextSettings (MSAA samples, depth/stencil format)
- Optimize command buffer recording (secondary command buffers)
- Add GPU resource caching (pipelines, descriptor sets)
- Improve error handling (validation layer messages)
- Add profiling markers (Vulkan debug utils, WebGPU labels)

### Requires Coordination

- Changing frame lifecycle affects Application::update() loop
- Modifying RenderContext API breaks window/ integration
- Altering backend selection logic affects CMake platform conditionals
- Adding new RendererAPIType requires factory pattern update
- Changing swapchain format affects shader outputs

### Almost Never Change

- **Frame lifecycle order** — beginFrame → endFrame is load-bearing for synchronization
- **Backend exclusivity** — running both Vulkan + WebGPU simultaneously breaks
- **Factory pattern** — RenderSystem::create() is standard creation path
- **VMA encapsulation** — exposing VMA directly breaks memory management abstraction
- **One context per window** — multi-context per window breaks synchronization

---

## Common Pitfalls

### Footguns

- ⚠️ **Calling drawScene() before beginFrame()**: Undefined behavior (command buffer not started)
- ⚠️ **Not recreating swapchain on resize**: Renders to wrong resolution or crashes
- ⚠️ **Missing fence/semaphore synchronization**: Frame-in-flight overlap causes corruption
- ⚠️ **Validation layers enabled in release**: 10-50x performance hit (check enableDebugLayers)
- ⚠️ **Destroying RenderContext while frame in-flight**: GPU crash (wait for idle before destroy)
- ⚠️ **Vulkan on Emscripten**: Compile error (WebGPU only on web)
- ⚠️ **WebGPU on Android**: Not supported (Vulkan only)

### Performance Traps

- 🐌 **Recreating pipelines every frame**: Compile shaders at startup (cache pipelines)
- 🐌 **Small draw calls**: High CPU overhead (batch geometry, use instancing)
- 🐌 **Synchronous resource uploads**: Blocks rendering (use staging buffers, upload async)
- 🐌 **Excessive swapchain images**: More memory, no performance gain (2-3 images sufficient)
- 🐌 **No descriptor set caching**: Recreating descriptor sets every frame is slow

### Historical Mistakes (Do NOT repeat)

- **Static Vulkan linking**: Switched to volk for dynamic loading (smaller binary, runtime backend selection)
- **No VMA**: Added VMA for GPU memory management (manual allocation was error-prone)
- **Single-threaded command recording**: Exploring secondary command buffers for parallelism

---

## How Claude Should Help

### Expected Tasks

- Add compute shader support (Vulkan compute pipelines, WebGPU compute passes)
- Implement descriptor set caching (reduce per-frame allocations)
- Add MSAA support (multisample attachments, resolve passes)
- Implement depth/stencil buffers (depth testing, stencil operations)
- Add GPU resource profiling (memory usage, draw call count)
- Write shader hot-reloading (watch .spv files, recompile on change)
- Optimize swapchain presentation (mailbox vs FIFO vs immediate)
- Add Vulkan secondary command buffers (parallel recording)
- Implement render graph (automatic resource barriers, execution order)

### Conservative Approach Required

- **Changing frame lifecycle**: Verify synchronization still correct (fences, semaphores)
- **Modifying VMA usage**: Risk of memory leaks or corruption (test thoroughly)
- **Altering backend factory**: May break platform-specific builds (test all platforms)
- **Removing validation layers**: Harder to debug (keep toggleable via settings)
- **Changing swapchain recreation logic**: May cause resize bugs (test extensively)

### Before Making Changes

- [ ] Verify backend compiles on target platform (Vulkan: Windows/Linux/Android, WebGPU: Emscripten)
- [ ] Test with validation layers enabled (catch Vulkan API misuse)
- [ ] Run on multiple GPUs (NVIDIA, AMD, Intel) - driver differences matter
- [ ] Test window resize, minimize, fullscreen (swapchain recreation edge cases)
- [ ] Verify frame-in-flight synchronization (no artifacts or crashes)
- [ ] Check VMA allocations/deallocations (no leaks via VMA stats)
- [ ] Test on web (Emscripten) if touching WebGPU backend

---

## Quick Reference

### Files

**Public API:**

- `include/saturn/graphics/render_system.hpp` — RenderSystem, RendererAPIType
- `include/saturn/graphics/render_context.hpp` — RenderContext, RenderContextSettings
- `include/saturn/graphics/*.hpp` — Buffer, Shader, Texture, Pipeline (public abstractions)

**Vulkan Backend (src/graphics/Vulkan/):**

- `context/` — VulkanInstance, VulkanDevice, VulkanSurface
- `commands/` — VulkanCommandPool, VulkanCommandBuffer, VulkanRenderPass
- `pipelines/` — VulkanPipeline, VulkanShaderModule
- `vulkan_render_system.{hpp,cpp}` — Vulkan RenderSystem implementation
- `vulkan_render_context.{hpp,cpp}` — Vulkan RenderContext implementation
- `vulkan_swapchain.{hpp,cpp}` — Swapchain management
- `vulkan_framebuffers.{hpp,cpp}` — Framebuffer objects
- `vulkan_allocator.{hpp,cpp}` — VMA wrapper
- `vulkan_common.hpp` — Shared Vulkan utilities

**WebGPU Backend (src/graphics/WebGPU/):**

- `webgpu_instance.{hpp,cpp}` — WebGPU instance
- `webgpu_device.{hpp,cpp}` — Device, adapter, queue
- `webgpu_swapchain.{hpp,cpp}` — Swapchain
- `webgpu_commands.{hpp,cpp}` — Command encoder, render pass
- `webgpu_pipeline.{hpp,cpp}` — Render pipeline
- `webgpu_render_system.{hpp,cpp}` — WebGPU RenderSystem implementation
- `webgpu_render_context.{hpp,cpp}` — WebGPU RenderContext implementation
- `webgpu_common.hpp` — Shared WebGPU utilities

**Tests:**

- None currently (tests needed for backend abstraction, swapchain recreation)

### Key Functions/Methods

- `RenderSystem::create(RendererAPIType)` → unique_ptr<RenderSystem> — Factory for backend
- `RenderSystem::createContext(Window*)` → Result<RenderContext\*, string> — Create context for window
- `RenderContext::render()` — Executes frame lifecycle (internal: begin → update → draw → end)
- `RenderContext::beginFrame()` — Acquire swapchain image, begin command buffer
- `RenderContext::drawScene()` — Record draw commands
- `RenderContext::endFrame()` — End command buffer, submit, present
- `RenderContext::resizeFramebuffer()` — Recreate swapchain on window resize

### Build Flags

- `SATURN_PLATFORM_EMSCRIPTEN` — Forces WebGPU backend (no Vulkan)
- `SATURN_ENABLE_VALIDATION_LAYERS` — Enables Vulkan validation (debug builds)

---

## Status Notes

**Stable** — Both Vulkan and WebGPU backends functional. High-level rendering API (materials, lights) not implemented (user code). Render graph and resource caching future work.
