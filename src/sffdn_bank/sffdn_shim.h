#pragma once

// Minimal stand-ins for the sfFDN types used by simd_biquad_bank.h, so this vendored kernel does
// not drag in sfFDN's types.h (which includes nlohmann/json.hpp).
// Vendored from https://github.com/Segfault1602/sfFDN at commit ac9666d.

#define SFFDN_NONBLOCKING

namespace sfFDN
{
/** @brief Biquad section coefficients, matching the scipy second-order-section layout. */
struct FilterCoefficients
{
    float b0, b1, b2, a0, a1, a2;

    FilterCoefficients Normalize() const
    {
        return {b0 / a0, b1 / a0, b2 / a0, 1.0f, a1 / a0, a2 / a0};
    }
};
} // namespace sfFDN
