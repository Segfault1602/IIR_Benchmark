#pragma once

#include "multichannel_filter.h"

#include <array>
#include <cstdint>
#include <span>
#include <vector>

#include <Accelerate/Accelerate.h>

class McVdspBiquadm : public MultiChannelFilter
{
  public:
    McVdspBiquadm() = default;
    ~McVdspBiquadm() override;

    void prepare(std::span<const std::array<float, 6>> sos, uint32_t channels, uint32_t stages) override;
    void process(std::span<const float> input, std::span<float> output, uint32_t channels,
                 uint32_t frames) override;
    const char* name() const override { return "vDSP_biquadm"; }

    McVdspBiquadm(const McVdspBiquadm&) = delete;
    McVdspBiquadm& operator=(const McVdspBiquadm&) = delete;
    McVdspBiquadm(McVdspBiquadm&&) = delete;
    McVdspBiquadm& operator=(McVdspBiquadm&&) = delete;

  private:
    vDSP_biquadm_Setup setup_ = nullptr;
    std::vector<const float*> input_ptrs_;
    std::vector<float*> output_ptrs_;
};
