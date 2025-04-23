set(FILTER_SOURCE cmsis_scalar_filter.cpp basic_filter.cpp)

set(EXTRA_LIBS)

if(APPLE)
    set(FILTER_SOURCE ${FILTER_SOURCE} vdsp_filter.cpp)
    set(EXTRA_LIBS ${EXTRA_LIBS} "-framework Accelerate")
endif()

add_executable(filter_benchmarks main.cpp ${FILTER_SOURCE})
target_link_libraries(filter_benchmarks PUBLIC nanobench cmsis_neon ${EXTRA_LIBS})
target_compile_options(filter_benchmarks PRIVATE -O3)

add_executable(check check.cpp ${FILTER_SOURCE})
target_link_libraries(check PUBLIC doctest cmsis_neon ${EXTRA_LIBS})

target_include_directories(check PRIVATE ${doctest_SOURCE_DIR}/doctest)
target_compile_options(check PRIVATE -fsanitize=address)
target_link_options(check PRIVATE -fsanitize=address)
