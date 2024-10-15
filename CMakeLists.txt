cmake_minimum_required(VERSION 3.22)
project(tiny_stl)

set(CMAKE_CXX_STANDARD 20)

add_executable(tiny_stl main.cpp
        SmartPtr.hpp)
add_subdirectory(test)
