#pragma once

#include "multichannel_filter.h"

#include <array>
#include <cstdint>
#include <span>
#include <vector>

class McScalarDf2t : public MultiChannelFilter
{
  public:
    McScalarDf2t() = default;
    ~McScalarDf2t() override = default;

    void prepare(std::span<const std::array<float, 6>> sos, uint32_t channels, uint32_t stages) override;
    void process(std::span<const float> input, std::span<float> output, uint32_t channels,
                 uint32_t frames) override;
    const char* name() const override;

  private:
    struct Coeffs
    {
        float b0;
        float b1;
        float b2;
        float a1;
        float a2;
    };

    uint32_t channels_ = 0;
    uint32_t stages_ = 0;
    std::vector<Coeffs> coeffs_;
    std::vector<float> state_;
};
