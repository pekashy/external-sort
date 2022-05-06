set(CMAKE_CXX_COMPILER "/usr/bin/g++")

cmake_minimum_required(VERSION 3.19)
project(external_sort)

set(CMAKE_CXX_STANDARD 14)

add_library(sort_lib external_sort.cpp)

add_executable(external_sort main.cpp)
add_dependencies(external_sort sort_lib)
target_link_libraries(external_sort sort_lib)

add_executable(input_generator input_generator.cpp)
