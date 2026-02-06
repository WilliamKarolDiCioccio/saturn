set (TEST_SOURCES)

add_executable(cross_project_tests ${TEST_SOURCES} "main.cpp")

target_link_libraries(cross_project_tests PRIVATE saturn GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main)

add_test(NAME cross_project_tests COMMAND test_projects_interaction)

include("${CMAKE_SOURCE_DIR}/cmake/ConfigureRuntimeDLLs.cmake")

configure_runtime_dlls(cross_project_tests)

include ("${CMAKE_SOURCE_DIR}/cmake/ConfigureForBuildType.cmake")

configure_for_build_type(cross_project_tests)
