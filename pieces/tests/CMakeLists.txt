set(TEST_SOURCES
    "unit/allocators_test.cpp"
    "unit/bitset_test.cpp"
    "unit/coroutines_test.cpp"
    "unit/result_test.cpp"
    "unit/sparse_set_test.cpp"
    "unit/spmc_snapshot_buffer_test.cpp")

add_executable(pieces_tests ${TEST_SOURCES} "main.cpp")

target_link_libraries(
  pieces_tests PRIVATE pieces GTest::gtest GTest::gtest_main GTest::gmock
                       GTest::gmock_main)

add_test(NAME pieces_tests COMMAND test_pieces)

include("${CMAKE_SOURCE_DIR}/cmake/ConfigureForBuildType.cmake")

configure_for_build_type(pieces_tests)
