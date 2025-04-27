set(FILTER_SOURCE cmsis_scalar_filter.cpp basic_filter.cpp)

set(EXTRA_LIBS)

if(APPLE)
    set(FILTER_SOURCE ${FILTER_SOURCE} vdsp_filter.cpp)
    set(EXTRA_LIBS ${EXTRA_LIBS} "-framework Accelerate")
endif()

add_executable(basic_filter_perf main.cpp basic_filter.cpp)
target_link_libraries(basic_filter_perf PUBLIC nanobench)
target_compile_options(basic_filter_perf PRIVATE -O3)
target_compile_definitions(basic_filter_perf PRIVATE -DBASIC_FILTER)

add_executable(cmsis_scalar_perf main.cpp cmsis_scalar_filter.cpp)
target_link_libraries(cmsis_scalar_perf PUBLIC cmsis_scalar nanobench)
target_compile_options(cmsis_scalar_perf PRIVATE -O3)
target_compile_definitions(cmsis_scalar_perf PRIVATE -DCMSIS_FILTER_SCALAR)

if(APPLE)
    add_executable(vdsp_perf main.cpp vdsp_filter.cpp)
    target_link_libraries(vdsp_perf PUBLIC nanobench "-framework Accelerate")
    target_compile_options(vdsp_perf PRIVATE -O3)
    target_compile_definitions(vdsp_perf PRIVATE -DVSP_FILTER)
endif()

if(NEON_FILTER)
    add_executable(neon_perf main.cpp cmsis_scalar_filter.cpp)
    target_link_libraries(neon_perf PUBLIC cmsis_neon nanobench)
    target_compile_options(neon_perf PRIVATE -O3)
    target_compile_definitions(neon_perf PRIVATE -DNEON_FILTER -DARM_MATH_NEON)
endif()

add_executable(check check.cpp ${FILTER_SOURCE})
target_link_libraries(check PUBLIC doctest cmsis_scalar ${EXTRA_LIBS})

target_include_directories(check PRIVATE ${doctest_SOURCE_DIR}/doctest)

