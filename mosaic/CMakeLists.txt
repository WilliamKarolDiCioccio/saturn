# ----------------------------------------
# Sources
# ----------------------------------------

set(FRAMEWORK_SRC
        # Core
        "src/core/logger.cpp"
        "src/core/application.cpp"
        "src/core/platform.cpp"
        "src/core/tracer.cpp"
        "src/core/timer.cpp"
        # Window
        "src/window/window.cpp"
        "src/window/window_system.cpp"
        # Input
        "src/input/input_system.cpp"
        "src/input/input_context.cpp"
        "src/input/sources/mouse_input_source.cpp"
        "src/input/sources/keyboard_input_source.cpp"
        "src/input/sources/text_input_source.cpp"
        # Graphics
        "src/graphics/render_context.cpp"
        "src/graphics/render_system.cpp"
        # External headers that need compilation
        "src/external/stb.cpp")

# Platform specific
if (ANDROID)
    list(
            APPEND
            FRAMEWORK_SRC
            "src/platform/AGDK/agdk_window.cpp"
            "src/platform/AGDK/agdk_window_system.cpp"
            "src/platform/AGDK/agdk_platform.cpp"
            "src/platform/AGDK/jni_helper.cpp"
            "src/external/game_activity.cpp")
else ()
    if (WIN32)
        list(APPEND FRAMEWORK_SRC "src/platform/Win32/win32_platform.cpp")
    elseif (EMSCRIPTEN)
        list(APPEND FRAMEWORK_SRC "src/platform/Emscripten/emscripten_platform.cpp")
    endif ()

    list(
            APPEND
            FRAMEWORK_SRC
            "src/platform/GLFW/glfw_window.cpp"
            "src/platform/GLFW/glfw_window_system.cpp"
            "src/platform/GLFW/glfw_keyboard_input_source.cpp"
            "src/platform/GLFW/glfw_mouse_input_source.cpp"
            "src/platform/GLFW/glfw_text_input_source.cpp")
endif ()

if (NOT EMSCRIPTEN)
    list(
            APPEND
            FRAMEWORK_SRC
            # Vulkan Backend
            "src/graphics/vulkan/vulkan_render_context.cpp"
            "src/graphics/vulkan/vulkan_render_system.cpp"
            "src/graphics/vulkan/context/vulkan_instance.cpp"
            "src/graphics/vulkan/context/vulkan_device.cpp"
            "src/graphics/vulkan/context/vulkan_surface.cpp"
            "src/graphics/vulkan/pipelines/vulkan_shader_module.cpp"
            "src/graphics/vulkan/pipelines/vulkan_pipeline.cpp"
            "src/graphics/vulkan/commands/vulkan_command_pool.cpp"
            "src/graphics/vulkan/commands/vulkan_command_buffer.cpp"
            "src/graphics/vulkan/commands/vulkan_render_pass.cpp"
            "src/graphics/vulkan/vulkan_allocator.cpp"
            "src/graphics/vulkan/vulkan_framebuffers.cpp"
            "src/graphics/vulkan/vulkan_swapchain.cpp"
            "src/external/vma.cpp")
endif ()

if (NOT ANDROID)
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
endif ()

# ----------------------------------------
# Target Definition
# ----------------------------------------
if (EMSCRIPTEN OR ANDROID)
    add_library(mosaic STATIC ${FRAMEWORK_SRC})
else ()
    add_library(mosaic SHARED ${FRAMEWORK_SRC})
    target_compile_definitions(mosaic PRIVATE _MOSAIC_BUILD_EXPORT_DLL)
endif ()

# ----------------------------------------
# Preprocessor Definitions
# ----------------------------------------

# Platform-specific Vulkan and GLFW defines
if (EMSCRIPTEN)
    target_compile_definitions(
            mosaic
            PUBLIC GLM_FORCE_PURE
            PRIVATE WEBGPU_CPP_IMPLEMENTATION)
else ()
    if (CMAKE_SYSTEM_NAME MATCHES "Windows")
        target_compile_definitions(
                mosaic
                PRIVATE WIN32_LEAN_AND_MEAN
                NOMINMAX
                GLFW_EXPOSE_NATIVE_WIN32
                VK_NO_PROTOTYPES
                VOLK_STATIC_DEFINES
                VK_USE_PLATFORM_WIN32_KHR
                WEBGPU_CPP_IMPLEMENTATION)
    elseif (CMAKE_SYSTEM_NAME MATCHES "Linux")
        target_compile_definitions(
                mosaic
                PRIVATE GLFW_EXPOSE_NATIVE_X11 VK_NO_PROTOTYPES VOLK_STATIC_DEFINES
                VK_USE_PLATFORM_XLIB_KHR WEBGPU_CPP_IMPLEMENTATION)
    elseif (CMAKE_SYSTEM_NAME MATCHES "Darwin")
        target_compile_definitions(
                mosaic
                PRIVATE GLFW_EXPOSE_NATIVE_COCOA VK_NO_PROTOTYPES VOLK_STATIC_DEFINES
                VK_USE_PLATFORM_MACOS_MVK WEBGPU_CPP_IMPLEMENTATION)
    elseif (CMAKE_SYSTEM_NAME MATCHES "Android")
        target_compile_definitions(
                mosaic PRIVATE VK_NO_PROTOTYPES VOLK_STATIC_DEFINES
                VK_USE_PLATFORM_ANDROID_KHR)
    endif ()
endif ()

# ----------------------------------------
# Include Paths
# ----------------------------------------

target_include_directories(
        mosaic
        PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src" "${Stb_INCLUDE_DIR}"
        PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

if (EMSCRIPTEN)
    target_include_directories(
            mosaic PRIVATE "$ENV{EMSDK}/upstream/emscripten/system/include")
endif ()

# ----------------------------------------
# Compile Options
# ----------------------------------------

if (EMSCRIPTEN)
    target_link_options(
            mosaic
            PRIVATE
            sUSE_GLFW=3
            -sUSE_WEBGPU=1
            -sASYNCIFY
            -sALLOW_MEMORY_GROWTH
            -sEXPORTED_RUNTIME_METHODS=['ccall','cwrap'])
endif ()

# ----------------------------------------
# Version Header
# ----------------------------------------

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/include/mosaic/version.h.in"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/mosaic/version.h")

# ----------------------------------------
# Precompiled Headers
# ----------------------------------------
target_precompile_headers(
        mosaic
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
        mosaic PUBLIC pieces utf8cpp::utf8cpp fmt::fmt-header-only glm::glm-header-only
        nlohmann_json::nlohmann_json)

if (EMSCRIPTEN)
    target_link_libraries(mosaic PRIVATE webgpu glfw glfw3webgpu)
elseif (ANDROID)
    target_link_libraries(
            mosaic PUBLIC log android jnigraphics game-activity::game-activity
            volk::volk GPUOpen::VulkanMemoryAllocator)
else ()
    find_path(COLORCONSOLE_INCLUDE_DIRS "colorconsole.hpp")
    target_include_directories(mosaic PRIVATE ${COLORCONSOLE_INCLUDE_DIRS})
    target_link_libraries(mosaic PRIVATE glfw glfw3webgpu webgpu volk::volk
            GPUOpen::VulkanMemoryAllocator)
endif ()

# ----------------------------------------
# Compiler options
# ----------------------------------------
include("${CMAKE_SOURCE_DIR}/cmake/ConfigureForBuildType.cmake")

configure_for_build_type(mosaic)

include("${CMAKE_SOURCE_DIR}/cmake/EnableSIMD.cmake")

if (NOT EMSCRIPTEN AND NOT ANDROID)
    enable_simd(mosaic)
endif ()

# ----------------------------------------
# Post-Build Step
# ----------------------------------------

if (NOT ANDROID)
    if (COMMAND target_copy_webgpu_binaries)
        target_copy_webgpu_binaries(mosaic)
    else ()
        message(ERROR "target_copy_webgpu_binaries() is not defined!")
    endif ()
endif ()
