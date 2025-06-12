# A reusable CMake module to detect SIMD capabilities using compiler feature
# testing rather than just header availability.
#
# Usage: include(cmake/SimdConfig.cmake) enable_simd(TARGET_NAME)

cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

include(CheckCXXSourceCompiles)
include(CMakePushCheckState)

# X86 SIMD definitions: levels, test code, flags, and macros
set(_simd_x86_levels SSE2 SSE4.2 AVX AVX2 AVX512F)

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  set(_simd_x86_flags /arch:SSE2 /arch:SSE4.2 /arch:AVX /arch:AVX2 /arch:AVX512)
else()
  set(_simd_x86_flags -msse2 -msse4.1 -mavx -mavx2 -mavx512f)
endif()

set(_simd_x86_defs SIMD_X86_SSE2 SIMD_X86_SSE4_1 SIMD_X86_AVX SIMD_X86_AVX2
                   SIMD_X86_AVX512F)

# Test source code for each SIMD level
set(_simd_x86_test_sse2
    "
#include <emmintrin.h>
int main() {
    __m128i a = _mm_setzero_si128();
    return 0;
}")

set(_simd_x86_test_sse4_1
    "
#include <smmintrin.h>
int main() {
    __m128i a = _mm_setzero_si128();
    int sum = _mm_extract_epi32(a, 0);
    return sum;
}")

set(_simd_x86_test_avx
    "
#include <immintrin.h>
int main() {
    __m256 a = _mm256_setzero_ps();
    return 0;
}")

set(_simd_x86_test_avx2
    "
#include <immintrin.h>
int main() {
    __m256i a = _mm256_setzero_si256();
    return 0;
}")

set(_simd_x86_test_avx512f
    "
#include <immintrin.h>
int main() {
    __m512 a = _mm512_setzero_ps();
    return 0;
}")

# ARM NEON definitions
set(_simd_arm_levels NEON)
set(_simd_arm_flags -mfpu=neon)
set(_simd_arm_defs SIMD_ARM_NEON)

set(_simd_arm_test_neon
    "
#include <arm_neon.h>
int main() {
    float32x4_t a = vdupq_n_f32(0);
    return 0;
}")

function(enable_simd target)
  # Print CPU info for diagnostic purposes
  message(STATUS "[SIMD] Target CPU: ${CMAKE_SYSTEM_PROCESSOR}")

  # Determine architecture and available tests
  if(CMAKE_SYSTEM_PROCESSOR MATCHES "(x86_64|AMD64|i[3-6]86)")
    set(_arch "x86")
    set(_levels ${_simd_x86_levels})
    set(_flags ${_simd_x86_flags})
    set(_defs ${_simd_x86_defs})
    message(
      STATUS "[SIMD] Detected x86 architecture: ${CMAKE_SYSTEM_PROCESSOR}")
  elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "(arm|aarch64)")
    set(_arch "arm")
    set(_levels ${_simd_arm_levels})
    set(_flags ${_simd_arm_flags})
    set(_defs ${_simd_arm_defs})
    message(
      STATUS "[SIMD] Detected ARM architecture: ${CMAKE_SYSTEM_PROCESSOR}")
  else()
    message(
      WARNING
        "[SIMD] Unknown architecture ${CMAKE_SYSTEM_PROCESSOR}, using scalar fallback for ${target}"
    )
    target_compile_definitions(${target} PRIVATE SIMD_SCALAR_FALLBACK)
    return()
  endif()

  list(LENGTH _levels _count)
  if(_count EQUAL 0)
    message(
      STATUS
        "[SIMD] No SIMD levels defined for this architecture—using scalar fallback for ${target}"
    )
    target_compile_definitions(${target} PRIVATE SIMD_SCALAR_FALLBACK)
    return()
  endif()

  # Calculate the last index properly
  math(EXPR _last_idx "${_count} - 1")

  # Start with the assumption that no SIMD is found
  set(_highest_level_idx -1)
  set(_highest_level "")
  set(_highest_flag "")
  set(_highest_def "")

  # Test each SIMD level and keep track of the highest supported one
  foreach(idx RANGE 0 ${_last_idx})
    list(GET _levels ${idx} level)
    list(GET _flags ${idx} flag)
    list(GET _defs ${idx} definition)

    # Get the test code for this level
    set(_test_var _simd_${_arch}_test_${level})
    string(TOLOWER ${_test_var} _test_var_lower)
    string(REPLACE "." "_" _test_var_clean ${_test_var_lower})

    # Perform the actual test
    cmake_push_check_state(RESET)
    set(CMAKE_REQUIRED_FLAGS "${flag}")
    set(CMAKE_REQUIRED_QUIET TRUE)

    check_cxx_source_compiles("${${_test_var_clean}}" CAN_USE_${level})

    # Log the result
    if(CAN_USE_${level})
      message(STATUS "[SIMD] ${level} supported with flag ${flag}")
      set(_highest_level_idx ${idx})
      set(_highest_level ${level})
      set(_highest_flag ${flag})
      set(_highest_def ${definition})
    else()
      message(STATUS "[SIMD] ${level} not supported")
    endif()

    cmake_pop_check_state()
  endforeach()

  # Apply the highest supported SIMD level
  if(_highest_level_idx GREATER -1)
    message(
      STATUS
        "[SIMD] Using highest supported SIMD level: ${_highest_level} for target ${target}"
    )
    target_compile_options(${target} PRIVATE ${_highest_flag})
    target_compile_definitions(${target} PRIVATE ${_highest_def})
  else()
    message(
      STATUS
        "[SIMD] No SIMD extensions detected—using scalar fallback for ${target}"
    )
    target_compile_definitions(${target} PRIVATE SIMD_SCALAR_FALLBACK)

    # Last resort - use architecture-based fallback
    if(_arch STREQUAL "x86" AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
      message(STATUS "[SIMD] Enabling SSE2 by default for x86_64 target")
      target_compile_options(${target} PRIVATE -msse2)
      target_compile_definitions(${target} PRIVATE SIMD_X86_SSE2)
    elseif(_arch STREQUAL "arm" AND CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
      message(STATUS "[SIMD] Enabling NEON by default for aarch64 target")
      target_compile_definitions(${target} PRIVATE SIMD_ARM_NEON)
      # Modern aarch64 has NEON by default, no extra flags needed
    endif()
  endif()

  # Output the final configuration
  message(STATUS "[SIMD] Configuration for ${target} complete")
endfunction()
