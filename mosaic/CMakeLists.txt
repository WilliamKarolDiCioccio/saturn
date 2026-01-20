# ----------------------------------------
# Sources
# ----------------------------------------

set(FRAMEWORK_SRC
    # Core
    "src/core/logger_default_sink.cpp"
    "src/core/application.cpp"
    "src/core/platform.cpp"
    "src/core/sys_info.cpp"
    "src/core/sys_console.cpp"
    "src/core/sys_ui.cpp"
    "src/core/timer.cpp"
    "src/core/cmd_line_parser.cpp"
    # Tools
    "src/tools/logger.cpp"
    "src/tools/tracer.cpp"
    # Execution
    "src/exec/thread_pool.cpp"
    # Window
    "src/window/window.cpp"
    "src/window/window_system.cpp"
    # Input
    "src/input/input_system.cpp"
    "src/input/input_context.cpp"
    "src/input/sources/mouse_input_source.cpp"
    "src/input/sources/keyboard_input_source.cpp"
    "src/input/sources/unified_text_input_source.cpp"
    # Graphics
    "src/graphics/render_context.cpp"
    "src/graphics/render_system.cpp"
    # External headers that need compilation
    "src/external/stb.cpp")

# Platform specific
if(ANDROID)
  list(
    APPEND
    FRAMEWORK_SRC
    "src/platform/AGDK/agdk_window.cpp"
    "src/platform/AGDK/agdk_window_system.cpp"
    "src/platform/AGDK/agdk_platform.cpp"
    "src/platform/AGDK/jni_helper.cpp"
    "src/platform/AGDK/agdk_sys_info.cpp"
    "src/platform/AGDK/agdk_sys_console.cpp"
    "src/platform/AGDK/agdk_sys_ui.cpp")
else()
  if(WIN32)
    list(
      APPEND
      FRAMEWORK_SRC
      "src/platform/Win32/win32_platform.cpp"
      "src/platform/Win32/win32_sys_info.cpp"
      "src/platform/Win32/win32_sys_console.cpp"
      "src/platform/Win32/win32_sys_ui.cpp"
      "src/platform/Win32/win32_unified_text_input_source.cpp")
  elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
    list(
      APPEND
      FRAMEWORK_SRC
      "src/platform/POSIX/posix_platform.cpp"
      "src/platform/POSIX/posix_sys_info.cpp"
      "src/platform/POSIX/posix_sys_console.cpp"
      "src/platform/POSIX/posix_sys_ui.cpp")
  elseif(EMSCRIPTEN)
    list(
      APPEND
      FRAMEWORK_SRC
      "src/platform/Emscripten/emscripten_platform.cpp"
      "src/platform/Emscripten/emscripten_sys_info.cpp"
      "src/platform/Emscripten/emscripten_sys_console.cpp"
      "src/platform/Emscripten/emscripten_sys_ui.cpp")
  endif()

  list(
    APPEND
    FRAMEWORK_SRC
    "src/platform/GLFW/glfw_window.cpp"
    "src/platform/GLFW/glfw_window_system.cpp"
    "src/platform/GLFW/glfw_keyboard_input_source.cpp"
    "src/platform/GLFW/glfw_mouse_input_source.cpp")
endif()

if(NOT EMSCRIPTEN)
  list(
    APPEND
    FRAMEWORK_SRC
    # Vulkan Backend
    "src/graphics/Vulkan/vulkan_render_context.cpp"
    "src/graphics/Vulkan/vulkan_render_system.cpp"
    "src/graphics/Vulkan/context/vulkan_instance.cpp"
    "src/graphics/Vulkan/context/vulkan_device.cpp"
    "src/graphics/Vulkan/context/vulkan_surface.cpp"
    "src/graphics/Vulkan/pipelines/vulkan_shader_module.cpp"
    "src/graphics/Vulkan/pipelines/vulkan_pipeline.cpp"
    "src/graphics/Vulkan/commands/vulkan_command_pool.cpp"
    "src/graphics/Vulkan/commands/vulkan_command_buffer.cpp"
    "src/graphics/Vulkan/commands/vulkan_render_pass.cpp"
    "src/graphics/Vulkan/vulkan_allocator.cpp"
    "src/graphics/Vulkan/vulkan_framebuffers.cpp"
    "src/graphics/Vulkan/vulkan_swapchain.cpp"
    "src/external/vma.cpp")
endif()

if(NOT ANDROID)
  list(
    APPEND
    FRAMEWORK_SRC
    # WebGPU Backend
    "src/graphics/WebGPU/webgpu_instance.cpp"
    "src/graphics/WebGPU/webgpu_device.cpp"
    "src/graphics/WebGPU/webgpu_render_context.cpp"
    "src/graphics/WebGPU/webgpu_render_system.cpp"
    "src/graphics/WebGPU/webgpu_commands.cpp"
    "src/graphics/WebGPU/webgpu_swapchain.cpp"
    "src/graphics/WebGPU/webgpu_pipeline.cpp")
endif()

# ----------------------------------------
# Target Definition
# ----------------------------------------
if(EMSCRIPTEN OR ANDROID)
  add_library(saturn STATIC ${FRAMEWORK_SRC})
else()
  add_library(saturn SHARED ${FRAMEWORK_SRC})
  target_compile_definitions(saturn PRIVATE _SATURN_BUILD_EXPORT_DLL)
endif()

# ----------------------------------------
# Preprocessor Definitions
# ----------------------------------------

# Platform-specific Vulkan and GLFW defines
if(EMSCRIPTEN)
  target_compile_definitions(
    saturn
    PUBLIC GLM_FORCE_PURE
    PRIVATE WEBGPU_CPP_IMPLEMENTATION)
else()
  if(CMAKE_SYSTEM_NAME MATCHES "Windows")
    target_compile_definitions(
      saturn
      PRIVATE WIN32_LEAN_AND_MEAN
              NOMINMAX
              GLFW_EXPOSE_NATIVE_WIN32
              VK_NO_PROTOTYPES
              VOLK_STATIC_DEFINES
              VK_USE_PLATFORM_WIN32_KHR
              WEBGPU_CPP_IMPLEMENTATION)
  elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
    target_compile_definitions(
      saturn
      PRIVATE GLFW_EXPOSE_NATIVE_X11 VK_NO_PROTOTYPES VOLK_STATIC_DEFINES
              VK_USE_PLATFORM_XLIB_KHR WEBGPU_CPP_IMPLEMENTATION)
  elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    target_compile_definitions(
      saturn
      PRIVATE GLFW_EXPOSE_NATIVE_COCOA VK_NO_PROTOTYPES VOLK_STATIC_DEFINES
              VK_USE_PLATFORM_MACOS_MVK WEBGPU_CPP_IMPLEMENTATION)
  elseif(CMAKE_SYSTEM_NAME MATCHES "Android")
    target_compile_definitions(
      saturn PRIVATE VK_NO_PROTOTYPES VOLK_STATIC_DEFINES
                     VK_USE_PLATFORM_ANDROID_KHR)
  endif()
endif()

# ----------------------------------------
# Include Paths
# ----------------------------------------

target_include_directories(
  saturn
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src" "${Stb_INCLUDE_DIR}"
  PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

if(EMSCRIPTEN)
  target_include_directories(
    saturn PRIVATE "$ENV{EMSDK}/upstream/emscripten/cache/sysroot/include")
endif()

# ----------------------------------------
# Compile Options
# ----------------------------------------

if(EMSCRIPTEN)
  target_link_options(saturn PRIVATE
                      -sEXPORTED_RUNTIME_METHODS=['ccall','cwrap'])
endif()

# ----------------------------------------
# Version Header
# ----------------------------------------

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/include/saturn/version.h.in"
               "${CMAKE_CURRENT_SOURCE_DIR}/include/saturn/version.h")

# ----------------------------------------
# Precompiled Headers
# ----------------------------------------
target_precompile_headers(
  saturn
  PRIVATE
  <vector>
  <string>
  <iostream>
  <algorithm>
  <unordered_map>
  <unordered_set>
  <functional>
  <memory>
  <array>
  <chrono>)

# ----------------------------------------
# Link Libraries
# ----------------------------------------

target_link_libraries(
  saturn PUBLIC pieces utf8cpp::utf8cpp fmt::fmt-header-only
                glm::glm-header-only nlohmann_json::nlohmann_json)

if(EMSCRIPTEN)
  target_link_libraries(saturn PRIVATE webgpu glfw glfw3webgpu)
elseif(ANDROID)
  target_link_libraries(
    saturn PUBLIC log android jnigraphics game-activity::game-activity
                  volk::volk GPUOpen::VulkanMemoryAllocator)
else()
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_libraries(saturn PRIVATE stdc++_libbacktrace)
  endif()

  find_path(COLORCONSOLE_INCLUDE_DIRS "colorconsole.hpp")
  target_include_directories(saturn PRIVATE ${COLORCONSOLE_INCLUDE_DIRS})
  target_link_libraries(saturn PRIVATE glfw glfw3webgpu webgpu volk::volk
                                       GPUOpen::VulkanMemoryAllocator)
endif()

# ----------------------------------------
# Compiler options
# ----------------------------------------
include("${CMAKE_SOURCE_DIR}/cmake/ConfigureForBuildType.cmake")

configure_for_build_type(saturn)

include("${CMAKE_SOURCE_DIR}/cmake/ConfigSIMD.cmake")

# ----------------------------------------
# Post-Build Step
# ----------------------------------------

if(NOT ANDROID)
  if(COMMAND target_copy_webgpu_binaries)
    target_copy_webgpu_binaries(saturn)
  else()
    message(ERROR "target_copy_webgpu_binaries() is not defined!")
  endif()
endif()
