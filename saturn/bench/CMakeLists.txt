set(BENCH_SOURCES "ecs_bench.cpp")

add_executable(saturn_benchmark ${BENCH_SOURCES} "main.cpp")

target_link_libraries(saturn_benchmark PRIVATE saturn benchmark::benchmark)

include("${CMAKE_SOURCE_DIR}/cmake/ConfigureForBuildType.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/ConfigureRuntimeDLLs.cmake")

configure_for_build_type(saturn_benchmark)
configure_runtime_dlls(saturn_benchmark)

include("${CMAKE_SOURCE_DIR}/cmake/ConfigureSIMD.cmake")
