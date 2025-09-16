set(TEST_SOURCES
  "unit/ecs_test.cpp"
  "unit/typeless_vector_test.cpp"
  "unit/typeless_sparse_set_test.cpp")

add_executable(mosaic_tests ${TEST_SOURCES} "main.cpp")

target_link_libraries(
  mosaic_tests PRIVATE mosaic GTest::gtest GTest::gtest_main GTest::gmock
                       GTest::gmock_main)

add_test(NAME mosaic_tests COMMAND test_mosaic)

add_custom_command(
  TARGET mosaic_tests
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:mosaic_tests>
          $<TARGET_FILE_DIR:mosaic_tests>
  COMMAND_EXPAND_LISTS)

include("${CMAKE_SOURCE_DIR}/cmake/ConfigureForBuildType.cmake")

configure_for_build_type(mosaic_tests)
