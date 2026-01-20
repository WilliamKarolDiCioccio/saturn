set(BENCH_SOURCES "ecs_bench.cpp")

add_executable(saturn_benchmark ${BENCH_SOURCES} "main.cpp")

target_link_libraries(saturn_benchmark PRIVATE saturn benchmark::benchmark)

include("${CMAKE_SOURCE_DIR}/cmake/ConfigureForBuildType.cmake")

configure_for_build_type(saturn_benchmark)

include("${CMAKE_SOURCE_DIR}/cmake/ConfigSIMD.cmake")
