set(BENCH_SOURCES "sparse_vs_map_bench.cpp")

add_executable(pieces_benchmark ${BENCH_SOURCES} "main.cpp")

target_link_libraries(pieces_benchmark PRIVATE pieces benchmark::benchmark)

include("${CMAKE_SOURCE_DIR}/cmake/ConfigureForBuildType.cmake")

configure_for_build_type(mosaic)

include("${CMAKE_SOURCE_DIR}/cmake/EnableSIMD.cmake")

if(NOT EMSCRIPTEN AND NOT ANDROID)
  enable_simd(pieces_benchmark)
endif()
