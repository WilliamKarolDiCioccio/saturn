set (TEST_SOURCES)

add_executable(cross_project_tests ${TEST_SOURCES} "main.cpp")

target_link_libraries(cross_project_tests PRIVATE saturn GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main)

add_test(NAME cross_project_tests COMMAND test_projects_interaction)

if(WIN32)
  add_custom_command(
    TARGET cross_project_tests
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:cross_project_tests>
            $<TARGET_FILE_DIR:cross_project_tests>
    COMMAND_EXPAND_LISTS)
endif()

include ("${CMAKE_SOURCE_DIR}/cmake/ConfigureForBuildType.cmake")

configure_for_build_type(cross_project_tests)