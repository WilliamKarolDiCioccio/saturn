set(CODEX_SOURCES
    "src/files_collector.cpp"
    "src/source_extractor.cpp"
    "src/preprocessor.cpp"
    "src/parser.cpp"
    "src/symbols_database.cpp"
    "src/include_graph.cpp"
    "src/analyzer.cpp"
)

add_library(codex STATIC ${CODEX_SOURCES})

target_include_directories(
  codex
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src"
  PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_link_libraries(codex PRIVATE pieces fmt::fmt-header-only unofficial::tree-sitter::tree-sitter tree-sitter-cpp)

target_precompile_headers(
  codex
  PRIVATE
  <vector>
  <string>
  <iostream>
  <algorithm>
  <unordered_map>
  <unordered_set>
  <functional>
  <memory>
  <array>
  <chrono>
  <tree_sitter/api.h>)

include("${CMAKE_SOURCE_DIR}/cmake/ConfigureForBuildType.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/ConfigureTestCoverageGeneration.cmake")

configure_for_build_type(codex)
enable_test_coverage(codex)
