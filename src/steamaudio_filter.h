#pragma once

#include "filter.h"

#include <iir.h>

#include <span>
#include <vector>

class SteamAudioFilter : public Filter
{
  public:
    SteamAudioFilter(size_t num_stage = 0);
    ~SteamAudioFilter() override = default;

    void process(std::span<const float> input, std::span<float> output) override;

  private:
    std::vector<ipl::IIRFilterer> filters_;
};