set(CMAKE_CXX_COMPILER "/usr/bin/g++")

cmake_minimum_required(VERSION 3.19)
project(external_sort)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS_DEBUG "-std=c++14 -D_NDEBUG -O3 -lpthread")

add_library(sort_lib external_sort.cpp)

add_executable(external_sort main.cpp)
add_dependencies(external_sort sort_lib)
target_link_libraries(external_sort sort_lib)

add_executable(input_generator input_generator.cpp)

# comment if tests not needed

find_package(GTest REQUIRED)
enable_testing()
add_executable(tests tests.cpp)
add_dependencies(tests sort_lib)
target_link_libraries(tests sort_lib gtest gtest_main pthread)
