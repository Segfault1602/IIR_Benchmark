#pragma once

#include "multichannel_filter.h"

#include <memory>
#include <vector>

/**
 * @brief Builds one SimdBiquadBank instance per SIMD width available on this target.
 *
 * simd.h resolves the vector width at compile time, so comparing SSE against AVX means compiling
 * the kernel more than once. On x86 this returns the 4-wide SSE bank and the 8-wide AVX bank;
 * elsewhere it returns the single kernel the compiler selected (NEON or scalar).
 */
std::vector<std::unique_ptr<MultiChannelFilter>> MakeSimdBankVariants();
