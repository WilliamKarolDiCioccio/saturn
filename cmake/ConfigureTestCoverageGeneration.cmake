# ConfigureTestCoverageGeneration.cmake
# CMake module for per-target test coverage instrumentation.
#
# Supports GCC (gcov), Clang (llvm-cov/gcov), and MSVC (OpenCppCoverage).
# Designed for use with GTest-based test executables.
#
# Usage:
#   include(cmake/ConfigureTestCoverageGeneration.cmake)
#   enable_test_coverage(target_name)
#   add_coverage_report(report_name TARGETS target1 target2 ...)

cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

# Option to enable/disable coverage globally
option(SATURN_ENABLE_COVERAGE "Enable code coverage instrumentation" OFF)

# Cache detection results
set(_COVERAGE_TOOLS_DETECTED
    ""
    CACHE INTERNAL "Cache for coverage tool detection")

# Detect available coverage tools on the system
function(_detect_coverage_tools)
  if(_COVERAGE_TOOLS_DETECTED)
    return()
  endif()

  set(_tools "")

  # Detect gcov (GCC)
  find_program(GCOV_PATH gcov)
  if(GCOV_PATH)
    list(APPEND _tools "gcov")
    message(STATUS "[Coverage] Found gcov: ${GCOV_PATH}")
  endif()

  # Detect llvm-cov (Clang)
  find_program(LLVM_COV_PATH llvm-cov)
  if(LLVM_COV_PATH)
    list(APPEND _tools "llvm-cov")
    message(STATUS "[Coverage] Found llvm-cov: ${LLVM_COV_PATH}")
  endif()

  # Detect lcov (report generation)
  find_program(LCOV_PATH lcov)
  if(LCOV_PATH)
    list(APPEND _tools "lcov")
    message(STATUS "[Coverage] Found lcov: ${LCOV_PATH}")
  endif()

  # Detect genhtml (HTML report generation)
  find_program(GENHTML_PATH genhtml)
  if(GENHTML_PATH)
    list(APPEND _tools "genhtml")
    message(STATUS "[Coverage] Found genhtml: ${GENHTML_PATH}")
  endif()

  # Detect OpenCppCoverage (MSVC on Windows)
  if(WIN32)
    find_program(OPENCPPCOVERAGE_PATH OpenCppCoverage)
    if(OPENCPPCOVERAGE_PATH)
      list(APPEND _tools "OpenCppCoverage")
      message(STATUS "[Coverage] Found OpenCppCoverage: ${OPENCPPCOVERAGE_PATH}")
    endif()
  endif()

  set(_COVERAGE_TOOLS_DETECTED
      "${_tools}"
      CACHE INTERNAL "")
endfunction()

# Enable coverage instrumentation on a target
function(enable_test_coverage target)
  if(NOT SATURN_ENABLE_COVERAGE)
    return()
  endif()

  _detect_coverage_tools()

  # GCC coverage flags
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(
      ${target}
      PRIVATE -fprofile-arcs
              -ftest-coverage
              -fno-inline
              -fno-inline-small-functions
              -fno-default-inline
              -O0
              -g)
    target_link_options(${target} PRIVATE --coverage)

    # Mark target as coverage-enabled
    set_target_properties(${target} PROPERTIES COVERAGE_ENABLED TRUE)
    set_target_properties(${target} PROPERTIES COVERAGE_TYPE "gcov")

    message(STATUS "[Coverage] Enabled gcov coverage for target: ${target}")

  # Clang coverage flags (gcov-compatible mode)
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
    # Use gcov-compatible mode for broader tool support
    target_compile_options(
      ${target}
      PRIVATE --coverage -fno-inline -fno-inline-small-functions -O0 -g)
    target_link_options(${target} PRIVATE --coverage)

    set_target_properties(${target} PROPERTIES COVERAGE_ENABLED TRUE)
    set_target_properties(${target} PROPERTIES COVERAGE_TYPE "llvm-gcov")

    message(STATUS "[Coverage] Enabled llvm coverage (gcov mode) for target: ${target}")

  # MSVC - no compile-time instrumentation, use OpenCppCoverage at runtime
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    # MSVC requires debug info for coverage tools
    target_compile_options(${target} PRIVATE /Zi /Od /DEBUG:FULL)
    target_link_options(${target} PRIVATE /DEBUG:FULL /PROFILE)

    set_target_properties(${target} PROPERTIES COVERAGE_ENABLED TRUE)
    set_target_properties(${target} PROPERTIES COVERAGE_TYPE "msvc")

    if(NOT OPENCPPCOVERAGE_PATH)
      message(
        WARNING
          "[Coverage] MSVC detected but OpenCppCoverage not found. "
          "Install from: https://github.com/OpenCppCoverage/OpenCppCoverage/releases"
      )
    endif()

    message(STATUS "[Coverage] Enabled MSVC coverage (requires OpenCppCoverage) for target: ${target}")

  else()
    message(WARNING "[Coverage] Unsupported compiler for coverage: ${CMAKE_CXX_COMPILER_ID}")
  endif()
endfunction()

# Enable source-based coverage for Clang (more accurate but requires llvm-cov)
function(enable_test_coverage_llvm_source target)
  if(NOT SATURN_ENABLE_COVERAGE)
    return()
  endif()

  if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
    message(WARNING "[Coverage] LLVM source-based coverage only available with Clang")
    enable_test_coverage(${target})
    return()
  endif()

  _detect_coverage_tools()

  target_compile_options(
    ${target}
    PRIVATE -fprofile-instr-generate
            -fcoverage-mapping
            -fno-inline
            -O0
            -g)
  target_link_options(${target} PRIVATE -fprofile-instr-generate)

  set_target_properties(${target} PROPERTIES COVERAGE_ENABLED TRUE)
  set_target_properties(${target} PROPERTIES COVERAGE_TYPE "llvm-source")

  message(STATUS "[Coverage] Enabled LLVM source-based coverage for target: ${target}")
endfunction()

# Add a coverage report target
# Usage: add_coverage_report(coverage_report TARGETS test_exe1 test_exe2)
function(add_coverage_report report_name)
  if(NOT SATURN_ENABLE_COVERAGE)
    return()
  endif()

  cmake_parse_arguments(COVERAGE "" "" "TARGETS;EXCLUDE" ${ARGN})

  if(NOT COVERAGE_TARGETS)
    message(FATAL_ERROR "[Coverage] No targets specified for coverage report")
  endif()

  _detect_coverage_tools()

  # Determine coverage type from first target
  list(GET COVERAGE_TARGETS 0 _first_target)
  get_target_property(_coverage_type ${_first_target} COVERAGE_TYPE)

  if(NOT _coverage_type)
    message(WARNING "[Coverage] Target ${_first_target} not configured for coverage")
    return()
  endif()

  # Default exclusions
  set(_default_excludes
      "*/tests/*"
      "*/test/*"
      "*/bench/*"
      "*/benchmark/*"
      "*/third_party/*"
      "*/vcpkg_installed/*"
      "/usr/*"
      "*/gtest/*"
      "*/gmock/*")

  list(APPEND _excludes ${_default_excludes} ${COVERAGE_EXCLUDE})

  # GCC/Clang gcov-compatible coverage report using lcov
  if(_coverage_type MATCHES "gcov|llvm-gcov")
    if(NOT LCOV_PATH)
      message(WARNING "[Coverage] lcov not found, cannot create coverage report")
      return()
    endif()

    # Build exclude arguments
    set(_lcov_excludes "")
    foreach(_excl ${_excludes})
      list(APPEND _lcov_excludes "--exclude" "${_excl}")
    endforeach()

    # Create coverage report target
    add_custom_target(
      ${report_name}
      COMMENT "Generating coverage report: ${report_name}"
      # Reset counters
      COMMAND ${LCOV_PATH} --zerocounters --directory ${CMAKE_BINARY_DIR}
      # Run tests (user should run tests before this, but we capture baseline)
      COMMAND ${LCOV_PATH} --capture --initial --directory ${CMAKE_BINARY_DIR}
              --output-file ${report_name}_base.info
      # Capture coverage after test run
      COMMAND ${LCOV_PATH} --capture --directory ${CMAKE_BINARY_DIR}
              --output-file ${report_name}_test.info
      # Combine traces
      COMMAND ${LCOV_PATH} --add-tracefile ${report_name}_base.info
              --add-tracefile ${report_name}_test.info
              --output-file ${report_name}_total.info
      # Remove unwanted paths
      COMMAND ${LCOV_PATH} --remove ${report_name}_total.info ${_lcov_excludes}
              --output-file ${report_name}.info
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

    # HTML report generation
    if(GENHTML_PATH)
      add_custom_target(
        ${report_name}_html
        COMMENT "Generating HTML coverage report"
        COMMAND ${GENHTML_PATH} ${report_name}.info
                --output-directory ${report_name}_html
                --title "${PROJECT_NAME} Coverage"
                --legend --show-details
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS ${report_name})

      message(STATUS "[Coverage] Created target: ${report_name}_html (HTML report)")
    endif()

    message(STATUS "[Coverage] Created target: ${report_name} (lcov report)")

  # LLVM source-based coverage report
  elseif(_coverage_type STREQUAL "llvm-source")
    if(NOT LLVM_COV_PATH)
      message(WARNING "[Coverage] llvm-cov not found, cannot create coverage report")
      return()
    endif()

    find_program(LLVM_PROFDATA_PATH llvm-profdata)
    if(NOT LLVM_PROFDATA_PATH)
      message(WARNING "[Coverage] llvm-profdata not found, cannot create coverage report")
      return()
    endif()

    # Build target binary paths
    set(_target_binaries "")
    foreach(_target ${COVERAGE_TARGETS})
      list(APPEND _target_binaries "-object" "$<TARGET_FILE:${_target}>")
    endforeach()

    add_custom_target(
      ${report_name}
      COMMENT "Generating LLVM coverage report: ${report_name}"
      # Merge profraw files
      COMMAND ${LLVM_PROFDATA_PATH} merge -sparse
              ${CMAKE_BINARY_DIR}/default.profraw
              -o ${report_name}.profdata
      # Generate report
      COMMAND ${LLVM_COV_PATH} report ${_target_binaries}
              -instr-profile=${report_name}.profdata
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

    # HTML report
    add_custom_target(
      ${report_name}_html
      COMMENT "Generating LLVM HTML coverage report"
      COMMAND ${LLVM_COV_PATH} show ${_target_binaries}
              -instr-profile=${report_name}.profdata
              -format=html -output-dir=${report_name}_html
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
      DEPENDS ${report_name})

    message(STATUS "[Coverage] Created target: ${report_name} (llvm-cov report)")
    message(STATUS "[Coverage] Created target: ${report_name}_html (HTML report)")

  # MSVC coverage using OpenCppCoverage
  elseif(_coverage_type STREQUAL "msvc")
    if(NOT OPENCPPCOVERAGE_PATH)
      message(
        WARNING
          "[Coverage] OpenCppCoverage not found. "
          "Install from: https://github.com/OpenCppCoverage/OpenCppCoverage/releases")
      return()
    endif()

    # Build source filter arguments
    set(_occ_sources "")
    foreach(_target ${COVERAGE_TARGETS})
      list(APPEND _occ_sources "--sources" "${CMAKE_SOURCE_DIR}")
    endforeach()

    # Build exclude arguments
    set(_occ_excludes "")
    foreach(_excl ${_excludes})
      list(APPEND _occ_excludes "--excluded_sources" "${_excl}")
    endforeach()

    # Get test executable path (first target)
    list(GET COVERAGE_TARGETS 0 _test_target)

    add_custom_target(
      ${report_name}
      COMMENT "Generating OpenCppCoverage report: ${report_name}"
      COMMAND ${OPENCPPCOVERAGE_PATH}
              ${_occ_sources}
              ${_occ_excludes}
              --export_type=cobertura:${report_name}.xml
              --export_type=html:${report_name}_html
              -- "$<TARGET_FILE:${_test_target}>"
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

    message(STATUS "[Coverage] Created target: ${report_name} (OpenCppCoverage report)")
  endif()
endfunction()

# Convenience function: setup coverage for a GTest executable
function(setup_gtest_coverage test_target)
  cmake_parse_arguments(COVERAGE "LLVM_SOURCE" "REPORT_NAME" "EXCLUDE" ${ARGN})

  if(NOT COVERAGE_REPORT_NAME)
    set(COVERAGE_REPORT_NAME "${test_target}_coverage")
  endif()

  # Enable coverage on test target
  if(COVERAGE_LLVM_SOURCE)
    enable_test_coverage_llvm_source(${test_target})
  else()
    enable_test_coverage(${test_target})
  endif()

  # Create report target
  add_coverage_report(${COVERAGE_REPORT_NAME}
    TARGETS ${test_target}
    EXCLUDE ${COVERAGE_EXCLUDE})
endfunction()

# Print coverage configuration summary
function(print_coverage_summary)
  if(NOT SATURN_ENABLE_COVERAGE)
    message(STATUS "[Coverage] Coverage disabled (SATURN_ENABLE_COVERAGE=OFF)")
    return()
  endif()

  _detect_coverage_tools()

  message(STATUS "[Coverage] === Coverage Configuration ===")
  message(STATUS "[Coverage] Compiler: ${CMAKE_CXX_COMPILER_ID}")
  message(STATUS "[Coverage] Tools found: ${_COVERAGE_TOOLS_DETECTED}")

  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    message(STATUS "[Coverage] Method: gcov (--coverage)")
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
    message(STATUS "[Coverage] Method: llvm-cov (gcov-compatible or source-based)")
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    message(STATUS "[Coverage] Method: OpenCppCoverage (runtime instrumentation)")
  endif()

  message(STATUS "[Coverage] ==============================")
endfunction()
