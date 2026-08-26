#pragma once

#include "multichannel_filter.h"

#include <arm_math.h>
#include <array>
#include <span>
#include <vector>

class McCmsis : public MultiChannelFilter
{
  public:
    McCmsis() = default;
    ~McCmsis() override = default;

    void prepare(std::span<const std::array<float, 6>> sos, uint32_t channels, uint32_t stages) override;
    void process(std::span<const float> input, std::span<float> output, uint32_t channels, uint32_t frames) override;
    const char* name() const override;

  private:
    std::vector<arm_biquad_cascade_df2T_instance_f32> instances_;
    std::vector<std::vector<float>> coeffs_;
    std::vector<std::vector<float>> states_;
    uint32_t stages_ = 0;
};
