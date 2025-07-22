#pragma once

#include "filter.h"

#include <kfr/dsp/iir.hpp>

#include <vector>

class KfrFilter : public Filter
{
  public:
    KfrFilter(size_t num_stage = 0);
    ~KfrFilter() override;

    void process(std::span<const float> input, std::span<float> output) override;

  private:
    kfr::iir_filter<float>* biquad_instance_;
    std::vector<float> states_;
    std::vector<float> coeffs_;
    size_t stage_;
};