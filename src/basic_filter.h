#pragma once

#include "filter.h"

#include <span>
#include <vector>

class BiquadFilter
{
  public:
    BiquadFilter();
    ~BiquadFilter() = default;

    void SetCoeffs(std::span<const float> coeffs);

    void process(std::span<const float> input, std::span<float> output);

  private:
    float a1_, a2_, b0_, b1_, b2_;
    float x0_, x1_, x2_, y0_, y1_, y2_;
};

class BasicFilter : public Filter
{
  public:
    BasicFilter();
    ~BasicFilter() override = default;

    void process(std::span<const float> input, std::span<float> output) override;

  private:
    std::vector<BiquadFilter> filters_;
};

class CascadedIIRDF2T : public Filter
{
  public:
    CascadedIIRDF2T();
    ~CascadedIIRDF2T() override = default;

    void process(std::span<const float> input, std::span<float> output) override;

  private:
    size_t stage_;

    std::vector<float> states_;
    std::vector<float> coeffs_;
};

class CascadedIIRDF1 : public Filter
{
  public:
    CascadedIIRDF1();
    ~CascadedIIRDF1() override = default;

    void process(std::span<const float> input, std::span<float> output) override;

  private:
    size_t stage_;

    std::vector<float> states_;
    std::vector<float> coeffs_;
};