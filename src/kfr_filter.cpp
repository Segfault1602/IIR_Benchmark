#include "kfr_filter.h"

#include "filter_coeffs.h"

KfrFilter::KfrFilter(size_t num_stage)
{
    stage_ = num_stage == 0 ? kTestSOS.size() : num_stage;
    coeffs_.resize(stage_ * 6);

    // KFR expects coefficients in the format:
    // a0, a1, a2, b0, b1, b2
    for (size_t i = 0; i < stage_; i++)
    {
        coeffs_[i * 6 + 0] = kTestSOS[i % kTestSOS.size()][3];
        coeffs_[i * 6 + 1] = kTestSOS[i % kTestSOS.size()][4];
        coeffs_[i * 6 + 2] = kTestSOS[i % kTestSOS.size()][5];
        coeffs_[i * 6 + 3] = -kTestSOS[i % kTestSOS.size()][0];
        coeffs_[i * 6 + 4] = -kTestSOS[i % kTestSOS.size()][1];
        coeffs_[i * 6 + 5] = -kTestSOS[i % kTestSOS.size()][2];
    }

    states_.resize(stage_ * 8, 0);
    biquad_instance_ = new kfr::iir_filter<float>(
        kfr::iir_params{reinterpret_cast<const kfr::biquad_section<float>*>(coeffs_.data()), stage_});
}

KfrFilter::~KfrFilter()
{
    delete biquad_instance_;
}

void KfrFilter::process(std::span<const float> input, std::span<float> output)
{
    biquad_instance_->apply(output.data(), input.data(), input.size());
}