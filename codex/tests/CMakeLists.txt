set(TEST_SOURCES
    "unit/parser_test.cpp"
    "unit/preprocessor_test.cpp"
)

add_executable(codex_tests ${TEST_SOURCES} "main.cpp")

target_link_libraries(
  codex_tests PRIVATE codex GTest::gtest GTest::gtest_main GTest::gmock
                       GTest::gmock_main)

add_test(NAME codex_tests COMMAND codex_tests)

include("${CMAKE_SOURCE_DIR}/cmake/ConfigureForBuildType.cmake")

configure_for_build_type(codex_tests)
