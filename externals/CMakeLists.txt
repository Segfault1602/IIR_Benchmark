set(CMSIS-DSP_FILTER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/CMSIS-DSP/Source/FilteringFunctions)

set(CMSIS_FILTER_SOURCE
    ${CMSIS-DSP_FILTER_DIR}/arm_biquad_cascade_df2T_f32.c
    ${CMSIS-DSP_FILTER_DIR}/arm_biquad_cascade_df2T_init_f32.c
    ${CMSIS-DSP_FILTER_DIR}/arm_biquad_cascade_df1_f32.c
    ${CMSIS-DSP_FILTER_DIR}/arm_biquad_cascade_df1_init_f32.c)

add_library(cmsis_scalar ${CMSIS_FILTER_SOURCE})
target_include_directories(cmsis_scalar PUBLIC CMSIS-DSP/Include)
target_compile_definitions(cmsis_scalar PRIVATE -DARM_MATH_LOOPUNROLL)
target_compile_options(cmsis_scalar PRIVATE -O3)

add_library(cmsis_neon ${CMSIS_FILTER_SOURCE})
target_include_directories(cmsis_neon PUBLIC CMSIS-DSP/Include)
target_compile_definitions(cmsis_neon PUBLIC -DARM_MATH_NEON -DARM_MATH_LOOPUNROLL)
target_compile_options(cmsis_neon PRIVATE -O3)
