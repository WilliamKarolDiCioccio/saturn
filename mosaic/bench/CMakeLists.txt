set(BENCH_SOURCES "ecs_bench.cpp")

add_executable(mosaic_benchmark ${BENCH_SOURCES} "main.cpp")

target_link_libraries(mosaic_benchmark PRIVATE mosaic benchmark::benchmark)

include("${CMAKE_SOURCE_DIR}/cmake/ConfigureForBuildType.cmake")

configure_for_build_type(mosaic_benchmark)

include("${CMAKE_SOURCE_DIR}/cmake/ConfigSIMD.cmake")
