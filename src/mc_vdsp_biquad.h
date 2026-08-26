#pragma once

#include "multichannel_filter.h"

#include <array>
#include <cstdint>
#include <span>
#include <vector>

#include <Accelerate/Accelerate.h>

class McVdspBiquad : public MultiChannelFilter
{
  public:
    McVdspBiquad() = default;
    ~McVdspBiquad() override;

    void prepare(std::span<const std::array<float, 6>> sos, uint32_t channels, uint32_t stages) override;
    void process(std::span<const float> input, std::span<float> output, uint32_t channels,
                 uint32_t frames) override;
    const char* name() const override { return "vDSP_biquad_xN"; }

    McVdspBiquad(const McVdspBiquad&) = delete;
    McVdspBiquad& operator=(const McVdspBiquad&) = delete;
    McVdspBiquad(McVdspBiquad&&) = delete;
    McVdspBiquad& operator=(McVdspBiquad&&) = delete;

  private:
    std::vector<vDSP_biquad_Setup> setups_;
    std::vector<std::vector<float>> delays_;
};
