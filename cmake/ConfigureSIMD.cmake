# CMake module for per-file SIMD optimization.
#
# Given a base source file and a set of SIMD instruction set architectures
# (ISAs), this module will automatically detect the host's compiler SIMD
# capabilities and compile the appropriate ISA-specific source files if they
# exist. At runtime, the correct SIMD code will be selected based on the
# detected capabilities.
#
# Usage: include(cmake/EnhancedSimdConfig.cmake)
# add_isa_specific_sources(target_name "filename.cpp" AVX SSE4_1)
#
# Available ISA flags: SSE2 SSE4_1 SSE4_2 AVX AVX2 AVX512F

cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

include(CheckCXXSourceCompiles)
include(CMakePushCheckState)

# X86 SIMD definitions: levels, test code, flags, and macros
set(_simd_x86_levels SSE2 SSE4_1 SSE4_2 AVX AVX2 AVX512F)

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  set(_simd_x86_flags_SSE2 "/arch:SSE2")
  set(_simd_x86_flags_SSE4_1 "/arch:SSE2") # MSVC doesn't have specific SSE4.1
                                           # flag
  set(_simd_x86_flags_SSE4_2 "/arch:SSE2") # MSVC doesn't have specific SSE4.2
                                           # flag
  set(_simd_x86_flags_AVX "/arch:AVX")
  set(_simd_x86_flags_AVX2 "/arch:AVX2")
  set(_simd_x86_flags_AVX512F "/arch:AVX512")
else()
  set(_simd_x86_flags_SSE2 "-msse2")
  set(_simd_x86_flags_SSE4_1 "-msse4.1")
  set(_simd_x86_flags_SSE4_2 "-msse4.2")
  set(_simd_x86_flags_AVX "-mavx")
  set(_simd_x86_flags_AVX2 "-mavx2")
  set(_simd_x86_flags_AVX512F "-mavx512f")
endif()

set(_simd_x86_defs_SSE2 "SIMD_X86_SSE2")
set(_simd_x86_defs_SSE4_1 "SIMD_X86_SSE4_1")
set(_simd_x86_defs_SSE4_2 "SIMD_X86_SSE4_2")
set(_simd_x86_defs_AVX "SIMD_X86_AVX")
set(_simd_x86_defs_AVX2 "SIMD_X86_AVX2")
set(_simd_x86_defs_AVX512F "SIMD_X86_AVX512F")

# File suffix mappings for ISA variants
set(_simd_x86_suffix_SSE2 "sse2")
set(_simd_x86_suffix_SSE4_1 "sse41")
set(_simd_x86_suffix_SSE4_2 "sse42")
set(_simd_x86_suffix_AVX "avx")
set(_simd_x86_suffix_AVX2 "avx2")
set(_simd_x86_suffix_AVX512F "avx512f")

# Test source code for each SIMD level
set(_simd_x86_test_sse2
    "#include <emmintrin.h>
int main() {
    __m128i a = _mm_setzero_si128();
    return 0;
}")

set(_simd_x86_test_sse4_1
    "#include <smmintrin.h>
int main() {
    __m128i a = _mm_setzero_si128();
    int sum = _mm_extract_epi32(a, 0);
    return sum;
}")

set(_simd_x86_test_sse4_2
    "#include <smmintrin.h>
int main() {
    __m128i a = _mm_setzero_si128();
    unsigned int crc = _mm_crc32_u32(0, 1);
    return crc;
}")

set(_simd_x86_test_avx
    "#include <immintrin.h>
int main() {
    __m256 a = _mm256_setzero_ps();
    return 0;
}")

set(_simd_x86_test_avx2
    "#include <immintrin.h>
int main() {
    __m256i a = _mm256_setzero_si256();
    return 0;
}")

set(_simd_x86_test_avx512f
    "#include <immintrin.h>
int main() {
    __m512 a = _mm512_setzero_ps();
    return 0;
}")

# ARM NEON definitions
set(_simd_arm_levels NEON)
set(_simd_arm_flags_NEON "-mfpu=neon")
set(_simd_arm_defs_NEON "SIMD_ARM_NEON")
set(_simd_arm_suffix_NEON "neon")

set(_simd_arm_test_neon
    "#include <arm_neon.h>
int main() {
    float32x4_t a = vdupq_n_f32(0);
    return 0;
}")

# Global variable to cache SIMD detection results
set(_SIMD_DETECTION_CACHE
    ""
    CACHE INTERNAL "Cache for SIMD detection results")

# Function to detect available SIMD capabilities on the host system
function(_detect_simd_capabilities)
  # Check if we already detected capabilities
  if(_SIMD_DETECTION_CACHE)
    return()
  endif()

  message(STATUS "[SIMD] Detecting host SIMD capabilities...")
  message(STATUS "[SIMD] Target CPU: ${CMAKE_SYSTEM_PROCESSOR}")

  # Determine architecture and available tests
  if(CMAKE_SYSTEM_PROCESSOR MATCHES "(x86_64|AMD64|i[3-6]86)")
    set(_arch "x86")
    set(_levels ${_simd_x86_levels})
    message(
      STATUS "[SIMD] Detected x86 architecture: ${CMAKE_SYSTEM_PROCESSOR}")
  elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "(arm|aarch64)")
    set(_arch "arm")
    set(_levels ${_simd_arm_levels})
    message(
      STATUS "[SIMD] Detected ARM architecture: ${CMAKE_SYSTEM_PROCESSOR}")
  else()
    message(WARNING "[SIMD] Unknown architecture ${CMAKE_SYSTEM_PROCESSOR}")
    set(_SIMD_DETECTION_CACHE
        "UNKNOWN_ARCH"
        CACHE INTERNAL "")
    return()
  endif()

  set(_supported_isas "")

  # Test each SIMD level
  foreach(level ${_levels})
    # Get test parameters for this level
    set(_test_var _simd_${_arch}_test_${level})
    string(TOLOWER ${_test_var} _test_var_lower)
    string(REPLACE "." "_" _test_var_clean ${_test_var_lower})

    set(_flag_var _simd_${_arch}_flags_${level})

    # Perform the compilation test
    cmake_push_check_state(RESET)
    set(CMAKE_REQUIRED_FLAGS "${${_flag_var}}")
    set(CMAKE_REQUIRED_QUIET TRUE)

    check_cxx_source_compiles("${${_test_var_clean}}" CAN_COMPILE_${level})

    if(CAN_COMPILE_${level})
      message(STATUS "[SIMD] ${level} compilation supported")
      list(APPEND _supported_isas ${level})
    else()
      message(STATUS "[SIMD] ${level} compilation not supported")
    endif()

    cmake_pop_check_state()
  endforeach()

  # Cache the results
  string(JOIN ";" _cache_value ${_arch} ${_supported_isas})
  set(_SIMD_DETECTION_CACHE
      "${_cache_value}"
      CACHE INTERNAL "")

  message(
    STATUS "[SIMD] Host compilation capabilities detected: ${_supported_isas}")
endfunction()

# Function to get the file suffix for a given ISA
function(_get_isa_suffix isa out_suffix)
  # Get current architecture from cache
  _detect_simd_capabilities()
  list(GET _SIMD_DETECTION_CACHE 0 _arch)

  if(_arch STREQUAL "x86")
    set(_suffix_var _simd_x86_suffix_${isa})
  elseif(_arch STREQUAL "arm")
    set(_suffix_var _simd_arm_suffix_${isa})
  else()
    set(${out_suffix}
        ""
        PARENT_SCOPE)
    return()
  endif()

  if(DEFINED ${_suffix_var})
    set(${out_suffix}
        "${${_suffix_var}}"
        PARENT_SCOPE)
  else()
    message(WARNING "[SIMD] Unknown ISA suffix for ${isa}")
    set(${out_suffix}
        ""
        PARENT_SCOPE)
  endif()
endfunction()

# Function to check if an ISA is supported for compilation
function(_is_isa_supported isa out_supported)
  _detect_simd_capabilities()

  if(NOT _SIMD_DETECTION_CACHE OR _SIMD_DETECTION_CACHE STREQUAL "UNKNOWN_ARCH")
    set(${out_supported}
        FALSE
        PARENT_SCOPE)
    return()
  endif()

  # Skip architecture part and get supported ISAs
  list(SUBLIST _SIMD_DETECTION_CACHE 1 -1 _supported_isas)

  if(${isa} IN_LIST _supported_isas)
    set(${out_supported}
        TRUE
        PARENT_SCOPE)
  else()
    set(${out_supported}
        FALSE
        PARENT_SCOPE)
  endif()
endfunction()

# Function to apply SIMD compilation flags to a source file
function(_apply_isa_flags target source_file isa)
  _detect_simd_capabilities()
  list(GET _SIMD_DETECTION_CACHE 0 _arch)

  if(_arch STREQUAL "x86")
    set(_flag_var _simd_x86_flags_${isa})
    set(_def_var _simd_x86_defs_${isa})
  elseif(_arch STREQUAL "arm")
    set(_flag_var _simd_arm_flags_${isa})
    set(_def_var _simd_arm_defs_${isa})
  else()
    return()
  endif()

  if(DEFINED ${_flag_var} AND DEFINED ${_def_var})
    # Apply compile options and definitions to the specific source file
    set_source_files_properties(
      ${source_file} PROPERTIES COMPILE_OPTIONS "${${_flag_var}}"
                                COMPILE_DEFINITIONS "${${_def_var}}")

    # For older CMake versions or compilers that need it, also try setting
    # compile flags This ensures compatibility across different CMake versions
    get_source_file_property(_existing_flags ${source_file} COMPILE_FLAGS)
    if(_existing_flags)
      set_source_files_properties(
        ${source_file} PROPERTIES COMPILE_FLAGS
                                  "${_existing_flags} ${${_flag_var}}")
    else()
      set_source_files_properties(${source_file} PROPERTIES COMPILE_FLAGS
                                                            "${${_flag_var}}")
    endif()

    message(
      STATUS
        "[SIMD] Applied ${isa} flags to ${source_file}: ${${_flag_var}} -D${${_def_var}}"
    )
  else()
    message(
      WARNING "[SIMD] Could not find flags or definitions for ISA: ${isa}")
  endif()
endfunction()

# Main function: add ISA-specific source variants to a target
function(add_isa_specific_sources target base_filename)
  set(_isa_list ${ARGN})

  if(NOT _isa_list)
    message(FATAL_ERROR "[SIMD] No ISAs specified for ${base_filename}")
  endif()

  # Always add the base file (scalar version)
  target_sources(${target} PRIVATE ${base_filename})
  message(STATUS "[SIMD] Added base source: ${base_filename}")

  # Extract file path and extension
  get_filename_component(_file_dir ${base_filename} DIRECTORY)
  get_filename_component(_file_name ${base_filename} NAME_WE)
  get_filename_component(_file_ext ${base_filename} EXT)

  # Process each ISA variant
  foreach(isa ${_isa_list})
    _is_isa_supported(${isa} _supported)

    if(_supported)
      _get_isa_suffix(${isa} _suffix)

      if(_suffix)
        # Construct the ISA-specific filename
        if(_file_dir)
          set(_isa_filename "${_file_dir}/${_file_name}_${_suffix}${_file_ext}")
        else()
          set(_isa_filename "${_file_name}_${_suffix}${_file_ext}")
        endif()

        # Check if the ISA-specific file exists
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_isa_filename}")
          target_sources(${target} PRIVATE ${_isa_filename})
          _apply_isa_flags(${target} ${_isa_filename} ${isa})
          message(STATUS "[SIMD] Added ISA-specific source: ${_isa_filename}")
        else()
          message(
            STATUS
              "[SIMD] ISA-specific source not found (skipping): ${_isa_filename}"
          )
        endif()
      endif()
    else()
      message(STATUS "[SIMD] ISA ${isa} not supported by compiler (skipping)")
    endif()
  endforeach()
endfunction()

# Legacy function for backward compatibility - detects and applies global SIMD
function(global_enable_simd target)
  message(
    STATUS
      "[SIMD] Legacy global_enable_simd called for ${target} - consider using add_isa_specific_sources for better control"
  )

  _detect_simd_capabilities()

  if(NOT _SIMD_DETECTION_CACHE OR _SIMD_DETECTION_CACHE STREQUAL "UNKNOWN_ARCH")
    message(
      WARNING
        "[SIMD] No SIMD support available - using scalar fallback for ${target}"
    )
    target_compile_definitions(${target} PRIVATE SIMD_SCALAR_FALLBACK)
    return()
  endif()

  # Get architecture and supported ISAs
  list(GET _SIMD_DETECTION_CACHE 0 _arch)
  list(SUBLIST _SIMD_DETECTION_CACHE 1 -1 _supported_isas)

  if(NOT _supported_isas)
    message(
      STATUS
        "[SIMD] No SIMD extensions detected - using scalar fallback for ${target}"
    )
    target_compile_definitions(${target} PRIVATE SIMD_SCALAR_FALLBACK)
    return()
  endif()

  # Use the highest supported SIMD level
  list(GET _supported_isas -1 _highest_isa)

  set(_flag_var _simd_${_arch}_flags_${_highest_isa})
  set(_def_var _simd_${_arch}_defs_${_highest_isa})

  target_compile_options(${target} PRIVATE ${${_flag_var}})
  target_compile_definitions(${target} PRIVATE ${${_def_var}})

  message(
    STATUS
      "[SIMD] Applied highest supported SIMD level ${_highest_isa} to target ${target}"
  )
endfunction()
