set(TEST_SOURCES
  "unit/ecs_test.cpp"
  "unit/typeless_vector_test.cpp"
  "unit/typeless_sparse_set_test.cpp"
  "unit/thread_pool_test.cpp")

add_executable(saturn_tests ${TEST_SOURCES} "main.cpp")

target_include_directories(saturn_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/mocks")

target_link_libraries(
  saturn_tests PRIVATE saturn GTest::gtest GTest::gtest_main GTest::gmock
                       GTest::gmock_main)

add_test(NAME saturn_tests COMMAND test_saturn)

if(WIN32)
  add_custom_command(
    TARGET saturn_tests
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:saturn_tests>
            $<TARGET_FILE_DIR:saturn_tests>
    COMMAND_EXPAND_LISTS)
endif()

include("${CMAKE_SOURCE_DIR}/cmake/ConfigureForBuildType.cmake")

configure_for_build_type(saturn_tests)
