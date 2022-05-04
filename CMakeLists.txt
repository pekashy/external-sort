set(CMAKE_CXX_COMPILER "/usr/bin/g++")

cmake_minimum_required(VERSION 3.19)
project(external_sort)

set(CMAKE_CXX_STANDARD 14)

add_executable(external_sort main.cpp)
add_executable(input_generator input_generator.cpp)
