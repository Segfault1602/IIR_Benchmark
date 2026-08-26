#include "steamaudio_kernel.h"

#include <context.h>

const char* SteamAudioKernelName()
{
    // Mirrors the dispatch in SteamAudio's IIR2Filterer constructor: the 8-wide kernel is only
    // compiled in on x86, and is only selected when the CPU actually reports AVX support.
#if defined(IPL_ENABLE_FLOAT8)
    if (ipl::gSIMDLevel() >= ipl::SIMDLevel::AVX)
    {
        return "AVX";
    }
#endif
#if defined(IPL_CPU_ARM64) || defined(IPL_CPU_ARMV7)
    return "NEON";
#else
    return "SSE";
#endif
}
