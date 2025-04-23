#include "cmsis_scalar_filter.h"

#include "filter_coeffs.h"

CMSISScalarFilterDF2T::CMSISScalarFilterDF2T()
{

    stage_ = kTestSOS.size();
    coeffs_.resize(stage_ * 5);

    for (size_t i = 0; i < kTestSOS.size(); i++)
    {
        coeffs_[i * 5 + 0] = kTestSOS[i][0] / kTestSOS[i][3];
        coeffs_[i * 5 + 1] = kTestSOS[i][1] / kTestSOS[i][3];
        coeffs_[i * 5 + 2] = kTestSOS[i][2] / kTestSOS[i][3];
        coeffs_[i * 5 + 3] = -kTestSOS[i][4] / kTestSOS[i][3];
        coeffs_[i * 5 + 4] = -kTestSOS[i][5] / kTestSOS[i][3];
    }

    states_.resize(stage_ * 8, 0);

#if defined(ARM_MATH_NEON)
    std::vector<float> compute_coeffs(stage_ * 8);
    arm_biquad_cascade_df2T_compute_coefs_f32(stage_, coeffs_.data(), compute_coeffs.data());
    coeffs_ = compute_coeffs;
#endif

    arm_biquad_cascade_df2T_init_f32(&biquad_instance_, stage_, coeffs_.data(), states_.data());
}

void CMSISScalarFilterDF2T::process(std::span<const float> input, std::span<float> output)
{
    arm_biquad_cascade_df2T_f32(&biquad_instance_, input.data(), output.data(), input.size());
}

CMSISScalarFilterDF1::CMSISScalarFilterDF1()
{
    stage_ = kTestSOS.size();
    coeffs_.resize(stage_ * 5);

    for (size_t i = 0; i < kTestSOS.size(); i++)
    {
        coeffs_[i * 5 + 0] = kTestSOS[i][0] / kTestSOS[i][3];
        coeffs_[i * 5 + 1] = kTestSOS[i][1] / kTestSOS[i][3];
        coeffs_[i * 5 + 2] = kTestSOS[i][2] / kTestSOS[i][3];
        coeffs_[i * 5 + 3] = -kTestSOS[i][4] / kTestSOS[i][3];
        coeffs_[i * 5 + 4] = -kTestSOS[i][5] / kTestSOS[i][3];
    }

    states_.resize(stage_ * 8, 0);

    arm_biquad_cascade_df1_init_f32(&biquad_instance_, stage_, coeffs_.data(), states_.data());
}

void CMSISScalarFilterDF1::process(std::span<const float> input, std::span<float> output)
{
    arm_biquad_cascade_df1_f32(&biquad_instance_, input.data(), output.data(), input.size());
}