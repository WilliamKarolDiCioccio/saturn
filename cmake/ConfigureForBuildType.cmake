# TargetCompileOptions.cmake A reusable CMake module to apply compile options
# and link options based on the build type
#
# Usage: include(cmake/TargetCompileOptions.cmake)
# configure_for_build_type(target_name)

cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

function(configure_for_build_type target)
  # Common warning flags to be applied to all build types
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    target_compile_options(
      ${target}
      PRIVATE -Wall
              -Wextra
              -Wpedantic
              -Wformat=2
              -Wformat-security
              -Wnon-virtual-dtor
              -Woverloaded-virtual
              -Wshadow
              -Wunused)
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(
      ${target}
      PRIVATE /W4
              /permissive-
              /w14640 # Enable strict type checking
              /w14265 # Class has virtual functions but destructor is not
                      # virtual
              /w14826 # Conversion from 'type1' to 'type2' is sign-extended
              /w14928 # Illegal copy-initialization
              /Zc:preprocessor # C++ 20 conformant preprocessor
    )
  elseif(EMSCRIPTEN)
    target_compile_options(
      ${target}
      PRIVATE -Wall
              -Wextra
              -Wpedantic
              -Wformat=2
              -Wformat-security
              -Wnon-virtual-dtor
              -Woverloaded-virtual
              -Wshadow
              -Wunused)
  endif()

  # Debug configuration
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")

    target_compile_options(${target} PRIVATE $<$<CONFIG:Debug>:-O0 -g>)
    target_compile_definitions(${target}
                               PRIVATE $<$<CONFIG:Debug>:SATURN_DEBUG_BUILD>)

  elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")

    target_compile_options(${target} PRIVATE $<$<CONFIG:Debug>:/Od /Z7 /RTC1
                                             /MDd>)
    target_compile_definitions(${target}
                               PRIVATE $<$<CONFIG:Debug>:SATURN_DEBUG_BUILD>)

  elseif(EMSCRIPTEN)

    target_compile_options(${target} PRIVATE $<$<CONFIG:Debug>:-O0 -g>)
    target_compile_definitions(${target}
                               PRIVATE $<$<CONFIG:Debug>:SATURN_DEBUG_BUILD>)
    target_link_options(${target} PRIVATE $<$<CONFIG:Debug>:-sASSERTIONS=2
                        -sSTACK_OVERFLOW_CHECK=2 -sSAFE_HEAP=1>)

  endif()

  # Dev configuration
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")

    target_compile_options(
      ${target} PRIVATE $<$<CONFIG:Dev>:-O2 -g -fstack-protector
                        -fno-omit-frame-pointer>)
    target_compile_definitions(${target}
                               PRIVATE $<$<CONFIG:Dev>:SATURN_DEV_BUILD>)
    target_link_options(${target} PRIVATE $<$<CONFIG:Dev>:-flto>)

  elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")

    target_compile_options(${target} PRIVATE $<$<CONFIG:Dev>:/GS /O2 /Ob1 /Zi>)
    target_compile_definitions(${target}
                               PRIVATE $<$<CONFIG:Dev>:SATURN_DEV_BUILD>)
    target_link_options(${target} PRIVATE $<$<CONFIG:Dev>:/LTCG>)

  elseif(EMSCRIPTEN)

    target_compile_options(
      ${target} PRIVATE $<$<CONFIG:Dev>:-O2 -g -fstack-protector
                        -fno-omit-frame-pointer>)
    target_compile_definitions(${target}
                               PRIVATE $<$<CONFIG:Dev>:SATURN_DEV_BUILD>)
    target_link_options(${target} PRIVATE $<$<CONFIG:Dev>:-flto -sASSERTIONS=1>)

  endif()

  # Release configuration
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")

    target_compile_options(
      ${target}
      PRIVATE $<$<CONFIG:Release>:-O3
              -Wconversion
              -Wsign-conversion
              -Wcast-qual
              -Wdouble-promotion
              -Wold-style-cast
              -Wdeprecated
              -Wundef
              -fstack-protector-strong
              -fno-common
              -D_FORTIFY_SOURCE=2
              -fPIE
              -fvisibility=hidden>)
    target_compile_definitions(
      ${target} PRIVATE $<$<CONFIG:Release>:SATURN_RELEASE_BUILD>)
    target_link_options(${target} PRIVATE $<$<CONFIG:Release>:-flto>)

  elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")

    target_compile_options(
      ${target}
      PRIVATE $<$<CONFIG:Release>:/GS
              /guard:cf
              /Qspectre
              /sdl
              /analyze
              /O2
              /Ob3
              /GL
              /Gy
              /we4289
              /w14905
              /w14906>)
    target_compile_definitions(
      ${target} PRIVATE $<$<CONFIG:Release>:SATURN_RELEASE_BUILD
                        _FORTIFY_SOURCE=2>)
    target_link_options(${target} PRIVATE $<$<CONFIG:Release>:/LTCG>)

  elseif(EMSCRIPTEN)

    target_compile_options(
      ${target}
      PRIVATE $<$<CONFIG:Release>:-O3
              -Wconversion
              -Wsign-conversion
              -Wcast-qual
              -Wdouble-promotion
              -Wold-style-cast
              -Wdeprecated
              -Wundef
              -fstack-protector-strong
              -fno-common
              -fvisibility=hidden>)
    target_compile_definitions(
      ${target} PRIVATE $<$<CONFIG:Release>:SATURN_RELEASE_BUILD>)
    target_link_options(${target} PRIVATE $<$<CONFIG:Release>:-flto>)

  endif()

  # Sanitizers configuration
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")

    target_compile_options(
      ${target}
      PRIVATE $<$<CONFIG:Sanitizers>:-g
              -O0
              -fno-omit-frame-pointer
              -fno-optimize-sibling-calls
              -fno-inline
              -fno-common
              -fms-runtime-lib=dll_dbg
              -fsanitize=address
              -fsanitize=undefined
              -fsanitize=thread>)
    target_compile_definitions(
      ${target} PRIVATE $<$<CONFIG:Sanitizers>:SATURN_SANITIZERS>)
    target_link_options(
      ${target} PRIVATE $<$<CONFIG:Sanitizers>:-fsanitize=address
      -fsanitize=undefined -fsanitize=thread>)

  elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")

    target_compile_options(
      ${target} PRIVATE $<$<CONFIG:Sanitizers>:/Zi /Od /RTC1 /DEBUG:FULL /MDd
                        /fsanitize=address>)
    target_compile_definitions(
      ${target} PRIVATE $<$<CONFIG:Sanitizers>:SATURN_SANITIZERS>)
    target_link_options(${target} PRIVATE
                        $<$<CONFIG:Sanitizers>:/INCREMENTAL:NO /DEBUG:FULL>)

  elseif(EMSCRIPTEN)

    target_compile_options(
      ${target}
      PRIVATE $<$<CONFIG:Sanitizers>:-g
              -O0
              -fno-omit-frame-pointer
              -fno-optimize-sibling-calls
              -fno-inline
              -fno-common
              -fsanitize=address
              -fsanitize=undefined>)
    target_compile_definitions(
      ${target} PRIVATE $<$<CONFIG:Sanitizers>:SATURN_SANITIZERS>)
    target_link_options(
      ${target}
      PRIVATE
      $<$<CONFIG:Sanitizers>:-fsanitize=address
      -fsanitize=undefined
      -sASSERTIONS=2
      -sSTACK_OVERFLOW_CHECK=2
      -sSAFE_HEAP=1>)

  endif()
endfunction()
