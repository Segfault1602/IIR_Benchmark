#include "mc_simd_bank_variants.h"

// Defined by the per-width object libraries in CMakeLists.txt. Each one compiles mc_simd_bank.cpp
// with `sfFDN` and `McSimdBank` renamed, so the two kernels can coexist in one binary without
// violating the ODR; the factory is the only symbol they expose.
#ifdef MC_HAS_SIMD_BANK_X86
std::unique_ptr<MultiChannelFilter> MakeSimdBankSse();
std::unique_ptr<MultiChannelFilter> MakeSimdBankAvx();
#else
std::unique_ptr<MultiChannelFilter> MakeSimdBankNative();
#endif

std::vector<std::unique_ptr<MultiChannelFilter>> MakeSimdBankVariants()
{
    std::vector<std::unique_ptr<MultiChannelFilter>> variants;
#ifdef MC_HAS_SIMD_BANK_X86
    variants.push_back(MakeSimdBankSse());
    variants.push_back(MakeSimdBankAvx());
#else
    variants.push_back(MakeSimdBankNative());
#endif
    return variants;
}
