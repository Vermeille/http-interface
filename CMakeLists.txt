cmake_minimum_required(VERSION 2.8.11)

project(HTTP-INTERFACE CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall -Wextra")

add_subdirectory(src)
add_subdirectory(example)

