cmake_minimum_required(VERSION 3.24...3.31 FATAL_ERROR)

# ---------------------------
# CMake Policies and Features
# ---------------------------
if(POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---------------------------
# Project Metadata
# ---------------------------
project(
  saturn_framework
  VERSION 0.0.1
  DESCRIPTION "A cross-platform game engine using modern C++"
  LANGUAGES C CXX)

# ---------------------------
# Compiler & C++ Configuration
# ---------------------------
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---------------------------
# MSVC-Specific Config
# ---------------------------
if(MSVC)
  add_compile_options(/utf-8)
endif()

# ---------------------------
# Emscripten-Specific Config
# ---------------------------
if(EMSCRIPTEN)
  set(CMAKE_EXECUTABLE_SUFFIX ".html")
  add_compile_options(-pthread)
  add_link_options(-pthread -sUSE_PTHREADS=1 -sPTHREAD_POOL_SIZE=2 -sASYNCIFY
                   -sALLOW_MEMORY_GROWTH -sDISABLE_EXCEPTION_CATCHING=0)
endif()

# ---------------------------
# Android-Specific Config
# ---------------------------
if(ANDROID)
  add_compile_options(-fexperimental-library)
endif()

# ---------------------------
# Dependencies via find_package
# ---------------------------
find_package(glm CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)
find_package(benchmark REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(utf8cpp CONFIG REQUIRED)
find_package(lyra CONFIG REQUIRED)
find_package(unofficial-tree-sitter CONFIG REQUIRED)

if(NOT EMSCRIPTEN)
  find_package(Vulkan REQUIRED)
  find_package(volk CONFIG REQUIRED)
  find_package(VulkanMemoryAllocator CONFIG REQUIRED)
endif()

if(ANDROID)
  find_package(game-activity REQUIRED CONFIG)
endif()

# ---------------------------
# Third-Party (Vendored) Libraries
# ---------------------------
if(NOT ANDROID)
  # Special case: GLFW handling
  if(EMSCRIPTEN)
    add_library(glfw INTERFACE)
    add_library(webgpu INTERFACE)
    target_link_options(glfw INTERFACE -sUSE_GLFW=3)
    target_link_options(webgpu INTERFACE --use-port=emdawnwebgpu)
  else()
    find_package(glfw3 CONFIG REQUIRED)
    add_subdirectory("vendor/webgpu")
  endif()

  add_subdirectory("vendor/glfw3webgpu")
  add_subdirectory("vendor/tree-sitter-cpp")

endif()

# ---------------------------
# Project Modules
# ---------------------------
add_subdirectory("pieces")
add_subdirectory("saturn")

if (NOT ANDROID AND NOT EMSCRIPTEN)
  add_subdirectory("codex")
  add_subdirectory("docsgen")
endif()

# ---------------------------
# Applications & Tests
# ---------------------------
add_subdirectory("testbed")

if(NOT ANDROID)
  add_subdirectory("saturn/tests")
  add_subdirectory("saturn/bench")
  add_subdirectory("pieces/tests")
  add_subdirectory("pieces/bench")
  add_subdirectory("codex/tests")
  add_subdirectory("tests")
endif()

# ---------------------------
# Enable Global Testing
# ---------------------------
enable_testing()
