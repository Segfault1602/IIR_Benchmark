#pragma once

#include "filter.h"
#include <span>
#include <vector>

#include <Accelerate/Accelerate.h>

class vDSPFilter : public Filter
{
  public:
    vDSPFilter();
    ~vDSPFilter() override = default;

    void process(std::span<const float> input, std::span<float> output) override;

  private:
    size_t stage_;

    std::vector<double> coeffs_;

    vDSP_biquad_Setup biquad_setup_ = nullptr;
    std::vector<float> delays_;
};