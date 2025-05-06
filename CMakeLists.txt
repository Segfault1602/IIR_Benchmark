cmake_minimum_required(VERSION 3.25)
set(CMAKE_CXX_STANDARD 20)

project(filter_benchmarks)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

message(STATUS ${CMAKE_SYSTEM_PROCESSOR})

# At lest until doctest updates their cmake file
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

option(NEON_FILTER "Enable NEON filter" OFF)

if((${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm64"))
    set(NEON_FILTER ON)
    message(STATUS "NEON filter enabled")
endif()

include(./fetch_deps.cmake)

find_package(IPP)
if(IPP_FOUND)
    message(STATUS "IPP found")
    set(USE_IPP ON)
else()
    message(STATUS "IPP not found")
    set(USE_IPP OFF)
endif()

add_subdirectory(externals)

add_subdirectory(src)
