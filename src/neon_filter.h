#pragma once

#include "filter.h"

#include <arm_neon.h>
#include <span>
#include <vector>

class BiquadNeonFilter
{
  public:
    BiquadNeonFilter();
    ~BiquadNeonFilter() = default;

    void SetCoeffs(std::span<const float, 5> coeffs);

    float process(float input);

    float32x4_t process(float32x4_t input);
    void process(std::span<const float> input, std::span<float> output);

  private:
    float32x4_t coeffs_[8];
    float xs[2];
    float ys[2];
    float b0_, b1_, b2_, a1_, a2_;
};

class NeonIIRFilter : public Filter
{
  public:
    NeonIIRFilter(size_t num_stage = 0);
    ~NeonIIRFilter() override = default;

    void process(std::span<const float> input, std::span<float> output) override;

  private:
    std::vector<BiquadNeonFilter> filters_;
};