# Builds just the IIR filter out of the SteamAudio sources fetched by fetch_deps.cmake.
#
# Upstream's own CMake build needs IPP, MKL, FLAC, mysofa and more, none of which this benchmark
# wants, so the sources are downloaded without being configured and only the closure required by
# IIR2Filterer is compiled here:
#
#   iir.cpp             the filter itself
#   float8_iir.cpp      its 8-wide AVX kernel (x86 only)
#   math_functions.cpp  Math::isFinite and friends, used by iir.cpp
#   bands.cpp           Bands::k*CutoffFrequencies, referenced by IIR2's band-filter helpers
#   simd_level.cpp      local stand-in for gSIMDLevel(), see the file for why

set(STEAMAUDIO_CORE ${steamaudio_SOURCE_DIR}/core/src/core)

if(NOT EXISTS ${STEAMAUDIO_CORE}/iir.cpp)
    message(FATAL_ERROR "SteamAudio sources not found at ${STEAMAUDIO_CORE}")
endif()

add_library(
    steamaudio_iir
    ${STEAMAUDIO_CORE}/iir.cpp
    ${STEAMAUDIO_CORE}/math_functions.cpp
    ${STEAMAUDIO_CORE}/bands.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/simd_level.cpp)

target_include_directories(steamaudio_iir PUBLIC ${STEAMAUDIO_CORE})
target_compile_features(steamaudio_iir PUBLIC cxx_std_20)
target_compile_options(steamaudio_iir PRIVATE -O3)

# IPL_ENABLE_FLOAT8 adds the mCoeffs8 member to the filterer in iir.h, so every translation unit
# that includes it has to agree on the layout: it must be PUBLIC. The kernel carries its own
# target("avx") attributes and is selected at run time from gSIMDLevel(), so enabling it does not
# stop the binary running on pre-AVX CPUs.
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|i686|i386")
    target_sources(steamaudio_iir PRIVATE ${STEAMAUDIO_CORE}/float8_iir.cpp)
    target_compile_definitions(steamaudio_iir PUBLIC IPL_ENABLE_FLOAT8)
endif()
