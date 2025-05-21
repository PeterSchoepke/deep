cmake_minimum_required(VERSION 3.24)
project(deep)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
set(CMAKE_CXX_STANDARD 17)

add_subdirectory(vendor)
add_subdirectory(src)