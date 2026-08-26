#pragma once

#include "multichannel_filter.h"

#include <array>
#include <kfr/dsp/iir.hpp>
#include <memory>
#include <span>
#include <vector>

class McKfr : public MultiChannelFilter
{
  public:
    McKfr() = default;
    ~McKfr() override = default;

    void prepare(std::span<const std::array<float, 6>> sos, uint32_t channels, uint32_t stages) override;
    void process(std::span<const float> input, std::span<float> output, uint32_t channels, uint32_t frames) override;
    const char* name() const override;

  private:
    // kfr::iir_params stores a POINTER to the coefficients rather than copying them, so the
    // backing storage must outlive the filter. Keeping one vector per channel here is what the
    // original single-channel kfr_filter.cpp does with its coeffs_ member.
    std::vector<std::vector<float>> coeffs_;
    std::vector<std::unique_ptr<kfr::iir_filter<float>>> filters_;
    uint32_t stages_ = 0;
};
