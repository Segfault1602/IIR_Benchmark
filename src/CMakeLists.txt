set(FILTER_SOURCE cmsis_filter.cpp basic_filter.cpp steamaudio_filter.cpp steamaudio_kernel.cpp
                  kfr_filter.cpp)
set(EXTRA_DEFINES "")
set(COMPILE_OPTION
    $<$<CONFIG:Release>:-O3>
    $<$<CONFIG:RelWithDebInfo>:-O3>
    $<$<CONFIG:Debug>:-Og>
    "-fno-signed-zeros"
    "-fno-trapping-math"
    "-fassociative-math"
    "-fno-math-errno"
    "-freciprocal-math"
    "-ftree-vectorize")

if(APPLE)
    set(FILTER_SOURCE ${FILTER_SOURCE} vdsp_filter.cpp)
    set(EXTRA_LIBS ${EXTRA_LIBS} "-framework Accelerate")
endif()

if(USE_IPP)
    set(FILTER_SOURCE ${FILTER_SOURCE} ipp_filter.cpp)
    set(EXTRA_LIBS ${EXTRA_LIBS} IPP::ippcore IPP::ipps)
    set(EXTRA_DEFINES ${EXTRA_DEFINES} -DIPP_FILTER)
endif()

add_executable(basic_filter_perf main.cpp basic_filter.cpp steamaudio_filter.cpp
                                steamaudio_kernel.cpp kfr_filter.cpp)
target_link_libraries(basic_filter_perf PUBLIC nanobench steamaudio_iir kfr kfr_dsp)
target_compile_options(basic_filter_perf PRIVATE "${COMPILE_OPTION}")
target_compile_definitions(basic_filter_perf PRIVATE -DBASIC_FILTER -DSTEAMAUDIO_FILTER)

add_executable(cmsis_scalar_perf main.cpp cmsis_filter.cpp)
target_link_libraries(cmsis_scalar_perf PUBLIC cmsis_scalar nanobench)
target_compile_options(cmsis_scalar_perf PRIVATE "${COMPILE_OPTION}")
target_compile_definitions(cmsis_scalar_perf PRIVATE -DCMSIS_FILTER_SCALAR)

if(APPLE)
    add_executable(vdsp_perf main.cpp vdsp_filter.cpp)
    target_link_libraries(vdsp_perf PUBLIC nanobench "-framework Accelerate")
    target_compile_options(vdsp_perf PRIVATE "${COMPILE_OPTION}")
    target_compile_definitions(vdsp_perf PRIVATE -DVDSP_FILTER)
endif()

if(NEON_FILTER)
    add_executable(neon_perf main.cpp cmsis_filter.cpp neon_filter.cpp)
    target_link_libraries(neon_perf PUBLIC cmsis_neon nanobench)
    target_compile_options(neon_perf PRIVATE "${COMPILE_OPTION}")
    target_compile_definitions(neon_perf PRIVATE -DCMSIS_FILTER_NEON -DARM_MATH_NEON -DNEON_FILTER)
endif()

if(USE_IPP)
    add_executable(ipp_perf main.cpp ipp_filter.cpp)
    target_link_libraries(ipp_perf PUBLIC IPP::ippcore IPP::ipps nanobench)
    target_compile_options(ipp_perf PRIVATE "${COMPILE_OPTION}")
    target_compile_definitions(ipp_perf PRIVATE -DIPP_FILTER)
endif()

add_executable(check check.cpp ${FILTER_SOURCE})
target_link_libraries(check PUBLIC doctest sndfile cmsis_scalar steamaudio_iir kfr kfr_dsp ${EXTRA_LIBS})
target_compile_definitions(check PRIVATE ${EXTRA_DEFINES} -DSTEAMAUDIO_FILTER)
target_include_directories(check PRIVATE ${doctest_SOURCE_DIR}/doctest)
if(APPLE)
    target_compile_options(check PRIVATE -fsanitize=address)
    target_link_options(check PRIVATE -fsanitize=address)
endif()

if(NEON_FILTER)
    add_executable(check_neon check.cpp neon_filter.cpp ${FILTER_SOURCE})
    target_link_libraries(check_neon PUBLIC doctest sndfile cmsis_neon steamaudio_iir kfr kfr_dsp ${EXTRA_LIBS})
    target_compile_definitions(check_neon PRIVATE -DCMSIS_FILTER_NEON -DARM_MATH_NEON -DNEON_FILTER)
    target_include_directories(check_neon PRIVATE ${doctest_SOURCE_DIR}/doctest)
endif()


if(NEON_FILTER)
    add_executable(neon_test neon_test.cpp neon_filter.cpp steamaudio_filter.cpp)
    target_link_libraries(neon_test PUBLIC steamaudio_iir)
endif()
# ==================== Multi-channel biquad bank benchmark ====================
# The single-channel Filter interface above cannot represent implementations that vectorize across
# channels, so these targets use the separate MultiChannelFilter interface. Per-channel libraries
# are instantiated N times, which is the realistic alternative to a true multi-channel bank.

set(MC_SOURCES
    mc_scalar.cpp
    mc_simd_bank_variants.cpp
    mc_kfr.cpp
    mc_steamaudio.cpp
    steamaudio_kernel.cpp)

# SimdBiquadBank's vector width is baked in by simd.h at compile time, so comparing SSE against AVX
# requires compiling the kernel once per width. Renaming the `sfFDN` namespace and the `McSimdBank`
# class per variant keeps both in one binary without an ODR violation, which matters because it lets
# every other backend be measured with identical flags in the same run.
function(sffdn_add_bank_variant variant)
    cmake_parse_arguments(ARG "" "" "FLAGS" ${ARGN})

    set(target mc_bank_${variant})
    add_library(${target} OBJECT mc_simd_bank.cpp sffdn_bank/audio_buffer.cpp)
    target_include_directories(
        ${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/sffdn_bank)
    target_compile_options(${target} PRIVATE "${COMPILE_OPTION}" ${ARG_FLAGS})
    target_compile_definitions(
        ${target}
        PRIVATE sfFDN=sfFDN_${variant} McSimdBank=McSimdBank${variant}
                MC_SIMD_BANK_FACTORY=MakeSimdBank${variant})
    target_compile_features(${target} PRIVATE cxx_std_20)
endfunction()

if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|i686|i386")
    # -mno-avx also switches off FMA, so the SSE variant is what a pre-AVX machine would run.
    sffdn_add_bank_variant(Sse FLAGS -msse4.2 -mno-avx)
    sffdn_add_bank_variant(Avx FLAGS -mavx2 -mfma)
    set(MC_BANK_OBJECTS $<TARGET_OBJECTS:mc_bank_Sse> $<TARGET_OBJECTS:mc_bank_Avx>)
    set(MC_DEFINES -DMC_HAS_SIMD_BANK_X86)
else()
    sffdn_add_bank_variant(Native)
    set(MC_BANK_OBJECTS $<TARGET_OBJECTS:mc_bank_Native>)
    set(MC_DEFINES "")
endif()

set(MC_LIBS steamaudio_iir kfr kfr_dsp)

if(APPLE)
    list(APPEND MC_SOURCES mc_vdsp_biquad.cpp mc_vdsp_biquadm.cpp)
    list(APPEND MC_LIBS "-framework Accelerate")
    list(APPEND MC_DEFINES -DMC_HAS_VDSP)
endif()

if(USE_IPP)
    list(APPEND MC_SOURCES mc_ipp.cpp)
    list(APPEND MC_LIBS IPP::ippcore IPP::ipps)
    list(APPEND MC_DEFINES -DMC_HAS_IPP)
endif()

function(sffdn_add_mc_target target_name)
    cmake_parse_arguments(ARG "NEON" "" "EXTRA_SOURCES;EXTRA_LIBS;EXTRA_DEFINES" ${ARGN})

    add_executable(${target_name} ${ARG_EXTRA_SOURCES} ${MC_SOURCES} ${MC_BANK_OBJECTS} mc_cmsis.cpp)
    target_link_libraries(${target_name} PUBLIC ${MC_LIBS} ${ARG_EXTRA_LIBS})
    target_compile_options(${target_name} PRIVATE "${COMPILE_OPTION}")
    target_compile_definitions(${target_name} PRIVATE ${MC_DEFINES} ${ARG_EXTRA_DEFINES})
    target_include_directories(
        ${target_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/sffdn_bank)
    # simd_biquad_bank.h uses std::span::first<N>() and a requires-clause.
    target_compile_features(${target_name} PRIVATE cxx_std_20)
endfunction()

sffdn_add_mc_target(
    multichannel_perf EXTRA_SOURCES mc_main.cpp EXTRA_LIBS nanobench cmsis_scalar)

sffdn_add_mc_target(
    multichannel_check EXTRA_SOURCES mc_check.cpp EXTRA_LIBS doctest cmsis_scalar)
target_include_directories(multichannel_check PRIVATE ${doctest_SOURCE_DIR}/doctest)

if(NEON_FILTER)
    sffdn_add_mc_target(
        multichannel_perf_neon
        EXTRA_SOURCES mc_main.cpp
        EXTRA_LIBS nanobench cmsis_neon
        EXTRA_DEFINES -DARM_MATH_NEON)
endif()
