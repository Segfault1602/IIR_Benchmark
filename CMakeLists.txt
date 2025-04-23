cmake_minimum_required(VERSION 3.25)
set(CMAKE_CXX_STANDARD 23)

project(filter_benchmarks)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(./fetch_deps.cmake)

add_subdirectory(externals)

add_subdirectory(src)
