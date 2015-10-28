cmake_minimum_required(VERSION 2.8.11)

project(HTTP-INTERFACE)
add_subdirectory(httpi)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -I..")
