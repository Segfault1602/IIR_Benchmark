cmake_minimum_required(VERSION 3.25)
set(CMAKE_CXX_STANDARD 23)

project(filter_benchmarks)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

message(STATUS ${CMAKE_SYSTEM_PROCESSOR})

# At lest until doctest updates their cmake file
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

option(NEON_FILTER "Enable NEON filter" OFF)

if ((${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch64") OR (${CMAKE_SYSTEM_PROCESSOR} MATCHES "armv8"))
    set(NEON_FILTER ON)
    message(STATUS "NEON filter enabled")
endif()

include(./fetch_deps.cmake)

add_subdirectory(externals)

add_subdirectory(src)
