add_library(pieces INTERFACE)

target_include_directories(pieces INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(pieces INTERFACE utf8cpp::utf8cpp)
