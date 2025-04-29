#include "vdsp_filter.h"

#include "filter_coeffs.h"

vDSPFilter::vDSPFilter()
{
    stage_ = kTestSOS.size() + 1;
    coeffs_.resize(stage_ * 5);

    for (size_t i = 0; i < kTestSOS.size(); i++)
    {
        coeffs_[i * 5 + 0] = kTestSOS[i][0] / kTestSOS[i][3];
        coeffs_[i * 5 + 1] = kTestSOS[i][1] / kTestSOS[i][3];
        coeffs_[i * 5 + 2] = kTestSOS[i][2] / kTestSOS[i][3];
        coeffs_[i * 5 + 3] = kTestSOS[i][4] / kTestSOS[i][3];
        coeffs_[i * 5 + 4] = kTestSOS[i][5] / kTestSOS[i][3];
    }

    // Add the dummy last stage
    coeffs_[(stage_ - 1) * 5 + 0] = 1.f;
    coeffs_[(stage_ - 1) * 5 + 1] = 0.f;
    coeffs_[(stage_ - 1) * 5 + 2] = 0.f;
    coeffs_[(stage_ - 1) * 5 + 3] = 0.f;
    coeffs_[(stage_ - 1) * 5 + 4] = 0.f;

    delays_.resize(stage_ * 2 + 2, 0);
    biquad_setup_ = vDSP_biquad_CreateSetup(coeffs_.data(), stage_);
}

void vDSPFilter::process(std::span<const float> input, std::span<float> output)
{
    vDSP_biquad(biquad_setup_, delays_.data(), input.data(), 1, output.data(), 1, input.size());
}