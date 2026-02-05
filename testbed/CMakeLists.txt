# ----------------------------------------
# Source Files
# ----------------------------------------

set(TESTBED_SRC "src/main.cpp" "src/testbed.cpp")

# ----------------------------------------
# Executable Target
# ----------------------------------------

if(ANDROID)
  add_library(testbed SHARED ${TESTBED_SRC})
else()
  add_executable(testbed ${TESTBED_SRC})
endif()

if(WIN32)
  set_property(TARGET testbed PROPERTY WIN32_EXECUTABLE TRUE)
endif()

# ----------------------------------------
# Dependencies
# ----------------------------------------

target_link_libraries(testbed PRIVATE saturn fmt::fmt)

# ----------------------------------------
# Emscripten Settings
# ----------------------------------------

if(EMSCRIPTEN)
  set_target_properties(testbed PROPERTIES SUFFIX ".html")
endif()

# ----------------------------------------
# Compiler options
# ----------------------------------------
include("${CMAKE_SOURCE_DIR}/cmake/ConfigureForBuildType.cmake")

configure_for_build_type(testbed)

include("${CMAKE_SOURCE_DIR}/cmake/ConfigureSIMD.cmake")

# ----------------------------------------
# Shader Compilation Setup
# ----------------------------------------

set(SHADER_INPUT_DIR "${CMAKE_SOURCE_DIR}/assets/shaders/vulkan")
set(TARGET_SHADER_DIR "$<TARGET_FILE_DIR:testbed>/shaders/bin")
set(ANDROID_SHADER_DIR
    "${CMAKE_SOURCE_DIR}/testbed_android/app/src/main/assets/shaders/bin")

if(ANDROID)
  if(CMAKE_HOST_SYSTEM_NAME MATCHES "Windows")
    add_custom_command(
      TARGET testbed
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E echo "Copying shaders to Android assets..."
      COMMAND
        powershell -ExecutionPolicy Bypass -File
        "${CMAKE_SOURCE_DIR}/scripts/compile_shaders.ps1" -InputDir
        "${SHADER_INPUT_DIR}" -CompileDir
        "${CMAKE_SOURCE_DIR}/assets/shaders/vulkan/bin" -OutputDir
        "${ANDROID_SHADER_DIR}"
      WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
  elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux" OR CMAKE_HOST_SYSTEM_NAME
                                                   MATCHES "Darwin")
    add_custom_command(
      TARGET testbed
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E echo "Copying shaders to Android assets..."
      COMMAND
        "${CMAKE_SOURCE_DIR}/scripts/compile_shaders.sh" "${SHADER_INPUT_DIR}"
        "${CMAKE_SOURCE_DIR}/assets/shaders/vulkan/bin" "${ANDROID_SHADER_DIR}"
      WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
  endif()
elseif(NOT EMSCRIPTEN)
  if(CMAKE_HOST_SYSTEM_NAME MATCHES "Windows")
    add_custom_command(
      TARGET testbed
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E echo "Compiling shaders using PowerShell..."
      COMMAND
        powershell -ExecutionPolicy Bypass -File
        "${CMAKE_SOURCE_DIR}/scripts/compile_shaders.ps1" -InputDir
        "${SHADER_INPUT_DIR}" -CompileDir
        "${CMAKE_SOURCE_DIR}/assets/shaders/vulkan/bin" -OutputDir
        "${TARGET_SHADER_DIR}"
      WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
  elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux" OR CMAKE_HOST_SYSTEM_NAME
                                                   MATCHES "Darwin")
    add_custom_command(
      TARGET testbed
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E echo "Compiling shaders using Bash..."
      COMMAND
        "${CMAKE_SOURCE_DIR}/scripts/compile_shaders.sh" "${SHADER_INPUT_DIR}"
        "${CMAKE_SOURCE_DIR}/assets/shaders/vulkan/bin" "${TARGET_SHADER_DIR}"
      WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
  endif()
endif()

# ----------------------------------------
# Copy Runtime DLLs (for Windows)
# ----------------------------------------

if(WIN32)
  add_custom_command(
    TARGET testbed
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:testbed>
            $<TARGET_FILE_DIR:testbed>
    COMMAND_EXPAND_LISTS)
endif()
