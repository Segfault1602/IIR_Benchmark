#pragma once

#include "filter.h"

#include <dsp/filtering_functions.h>
#include <vector>

class CMSISFilterDF2T : public Filter
{
  public:
    CMSISFilterDF2T(size_t num_stage = 0);
    ~CMSISFilterDF2T() override = default;

    void process(std::span<const float> input, std::span<float> output) override;

  private:
    arm_biquad_cascade_df2T_instance_f32 biquad_instance_;
    std::vector<float> states_;
    std::vector<float> coeffs_;
    size_t stage_;
};

class CMSISFilterDF1 : public Filter
{
  public:
    CMSISFilterDF1(size_t num_stage = 0);
    ~CMSISFilterDF1() override = default;

    void process(std::span<const float> input, std::span<float> output) override;

  private:
    arm_biquad_casd_df1_inst_f32 biquad_instance_;
    std::vector<float> states_;
    std::vector<float> coeffs_;
    size_t stage_;
};