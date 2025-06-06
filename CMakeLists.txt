cmake_minimum_required(VERSION 3.5)
project(zx)

enable_testing()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -export-dynamic")

# add_subdirectory(src)
add_subdirectory(tests)

include(dependencies.cmake)
