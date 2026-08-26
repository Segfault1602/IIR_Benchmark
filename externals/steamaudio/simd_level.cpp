// Provides the one symbol the SteamAudio IIR sources need from core/src/core/context.h.
//
// gSIMDLevel() lives in context.cpp upstream, which pulls in MKL, IPP, logging and the whole IPL
// Context object. All iir.cpp and float8_iir.cpp use it for is choosing between the 4-wide and
// 8-wide kernels, so this reports the CPU's actual capabilities directly instead.

#include "context.h"

namespace ipl
{

SIMDLevel gSIMDLevel()
{
#if (defined(IPL_CPU_X86) || defined(IPL_CPU_X64)) && (defined(__GNUC__) || defined(__clang__))
    __builtin_cpu_init();
    if (__builtin_cpu_supports("avx512f"))
    {
        return SIMDLevel::AVX512;
    }
    if (__builtin_cpu_supports("avx2"))
    {
        return SIMDLevel::AVX2;
    }
    if (__builtin_cpu_supports("avx"))
    {
        return SIMDLevel::AVX;
    }
    if (__builtin_cpu_supports("sse4.1"))
    {
        return SIMDLevel::SSE4;
    }
    return SIMDLevel::SSE2;
#elif defined(__AVX2__)
    return SIMDLevel::AVX2;
#elif defined(__AVX__)
    return SIMDLevel::AVX;
#else
    return SIMDLevel::SSE2;
#endif
}

} // namespace ipl
