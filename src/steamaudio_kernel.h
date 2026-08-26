#pragma once

/**
 * @brief Reports which SIMD kernel SteamAudio's filterer will actually use.
 *
 * Unlike the other backends, SteamAudio selects its kernel width at run time rather than compile
 * time, so benchmark labels have to be derived from the same condition iir.cpp dispatches on.
 * Returns "AVX", "SSE" or "NEON".
 */
const char* SteamAudioKernelName();
