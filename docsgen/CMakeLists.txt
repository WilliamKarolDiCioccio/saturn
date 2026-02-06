set(DOCSGEN_SOURCES
    "src/main.cpp"
    "src/cli/cli_parser.cpp"
    "src/generators/cross_linker.cpp"
    "src/generators/mdx_formatter.cpp"
    "src/generators/mdx_generator.cpp"
    "src/writers/file_writer.cpp"
)

add_executable(docsgen ${DOCSGEN_SOURCES})

target_include_directories(docsgen PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

target_link_libraries(docsgen PRIVATE codex bfg::lyra)

include("${CMAKE_SOURCE_DIR}/cmake/ConfigureForBuildType.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/ConfigureRuntimeDLLs.cmake")

configure_for_build_type(docsgen)
configure_runtime_dlls(docsgen)
