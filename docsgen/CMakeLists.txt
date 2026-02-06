set(CLI_SOURCES "src/main.cpp")

add_executable(docsgen ${CLI_SOURCES} "src/main.cpp")

target_link_libraries(docsgen PRIVATE codex bfg::lyra)

include("${CMAKE_SOURCE_DIR}/cmake/ConfigureForBuildType.cmake")

configure_for_build_type(docsgen)
configure_runtime_dlls(docsgen)
