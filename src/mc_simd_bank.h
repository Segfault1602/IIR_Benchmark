#pragma once

#include "multichannel_filter.h"
#include "sffdn_bank/simd_biquad_bank.h"

#include <array>
#include <cstdint>
#include <span>
#include <vector>

class McSimdBank : public MultiChannelFilter
{
  public:
    McSimdBank() = default;
    ~McSimdBank() override = default;

    void prepare(std::span<const std::array<float, 6>> sos, uint32_t channels, uint32_t stages) override;
    void process(std::span<const float> input, std::span<float> output, uint32_t channels,
                 uint32_t frames) override;
    const char* name() const override;

  private:
    sfFDN::SimdBiquadBank bank_;
    std::vector<sfFDN::FilterCoefficients> coeffs_;
    uint32_t channels_ = 0;
    uint32_t stages_ = 0;
};
