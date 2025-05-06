#include "vdsp_filter.h"

#include "filter_coeffs.h"

vDSPFilter::vDSPFilter(size_t num_stage)
{
    stage_ = num_stage == 0 ? kTestSOS.size() : num_stage;
    coeffs_.resize(stage_ * 5);

    for (size_t i = 0; i < stage_; i++)
    {
        coeffs_[i * 5 + 0] = kTestSOS[i % kTestSOS.size()][0] / kTestSOS[i % kTestSOS.size()][3];
        coeffs_[i * 5 + 1] = kTestSOS[i % kTestSOS.size()][1] / kTestSOS[i % kTestSOS.size()][3];
        coeffs_[i * 5 + 2] = kTestSOS[i % kTestSOS.size()][2] / kTestSOS[i % kTestSOS.size()][3];
        coeffs_[i * 5 + 3] = kTestSOS[i % kTestSOS.size()][4] / kTestSOS[i % kTestSOS.size()][3];
        coeffs_[i * 5 + 4] = kTestSOS[i % kTestSOS.size()][5] / kTestSOS[i % kTestSOS.size()][3];
    }

    delays_.resize(stage_ * 2 + 2, 0);
    biquad_setup_ = vDSP_biquad_CreateSetup(coeffs_.data(), stage_);
}

void vDSPFilter::process(std::span<const float> input, std::span<float> output)
{
    vDSP_biquad(biquad_setup_, delays_.data(), input.data(), 1, output.data(), 1, input.size());
}