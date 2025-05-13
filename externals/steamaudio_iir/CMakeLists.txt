set(STEAMAUDIO_IIR_SOURCE iir.cpp math_functions.cpp)

add_library(steamaudio_iir ${STEAMAUDIO_IIR_SOURCE})
target_include_directories(steamaudio_iir PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(steamaudio_iir PUBLIC -DIPL_CPU_ARM64)
