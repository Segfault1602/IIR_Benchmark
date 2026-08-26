#pragma once

#include "multichannel_filter.h"

#include <array>
#include <iir.h>
#include <span>
#include <vector>

class McSteamAudio : public MultiChannelFilter
{
  public:
    McSteamAudio() = default;
    ~McSteamAudio() override = default;

    void prepare(std::span<const std::array<float, 6>> sos, uint32_t channels, uint32_t stages) override;
    void process(std::span<const float> input, std::span<float> output, uint32_t channels,
                 uint32_t frames) override;
    const char* name() const override;

  private:
    std::vector<std::vector<ipl::IIR2Filterer>> filterers_;
    uint32_t stages_ = 0;
};
