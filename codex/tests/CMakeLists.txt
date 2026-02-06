set(TEST_SOURCES
    "unit/parser_test.cpp"
    "unit/preprocessor_test.cpp"
    "unit/analyzer_test.cpp"
)

add_executable(codex_tests ${TEST_SOURCES} "main.cpp")

target_link_libraries(
  codex_tests PRIVATE codex GTest::gtest GTest::gtest_main GTest::gmock
                       GTest::gmock_main)

add_test(NAME codex_tests COMMAND codex_tests)

include("${CMAKE_SOURCE_DIR}/cmake/ConfigureForBuildType.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/ConfigureTestCoverageGeneration.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/ConfigureRuntimeDLLs.cmake")

configure_for_build_type(codex_tests)
configure_runtime_dlls(codex_tests)
enable_test_coverage(codex_tests)

# Coverage report for codex library
add_coverage_report(codex_coverage
  TARGETS codex_tests
  EXCLUDE "*/codex/tests/*")
