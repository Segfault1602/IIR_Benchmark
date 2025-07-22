#include "neon_filter.h"

#include "filter_coeffs.h"
#include <cassert>

BiquadNeonFilter::BiquadNeonFilter()
{
    // Initialize coefficients to zero
    for (size_t i = 0; i < 8; ++i)
    {
        coeffs_[i] = vdupq_n_f32(0.0f);
    }

    b0_ = b1_ = b2_ = a1_ = a2_ = 0.0f;

    // Initialize state variables
    xs[0] = xs[1] = 0.0f;
    ys[0] = ys[1] = 0.0f;
}

void BiquadNeonFilter::SetCoeffs(std::span<const float, 5> coeffs)
{
    b0_ = coeffs[0];
    b1_ = coeffs[1];
    b2_ = coeffs[2];
    a1_ = coeffs[3];
    a2_ = coeffs[4];

    coeffs_[0] = {0.0f, 0.0f, 0.0f, b0_};
    coeffs_[1] = {0.0f, 0.0f, b0_, b1_};
    coeffs_[2] = {0.0f, b0_, b1_, b2_};
    coeffs_[3] = {b0_, b1_, b2_, 0.0f};
    coeffs_[4] = {b1_, b2_, 0.0f, 0.0f};
    coeffs_[5] = {b2_, 0.0f, 0.0f, 0.0f};
    coeffs_[6] = {-a1_, -a2_, 0.0f, 0.0f};
    coeffs_[7] = {-a2_, 0.0f, 0.0f, 0.0f};

    for (size_t i = 0; i < 8; ++i)
    {
        coeffs_[i][1] += -a1_ * coeffs_[i][0];
        coeffs_[i][2] += -a1_ * coeffs_[i][1] + -a2_ * coeffs_[i][0];
        coeffs_[i][3] += -a1_ * coeffs_[i][2] + -a2_ * coeffs_[i][1];
    }
}

float BiquadNeonFilter::process(float input)
{
    float y = b0_ * input + b1_ * xs[0] + b2_ * xs[1] - a1_ * ys[0] - a2_ * ys[1];
    xs[1] = xs[0];
    xs[0] = input;
    ys[1] = ys[0];
    ys[0] = y;
    return y;
}

float32x4_t BiquadNeonFilter::process(float32x4_t input)
{
    float32x4_t xm2 = vdupq_n_f32(xs[1]);
    float32x4_t xm1 = vdupq_n_f32(xs[0]);
    float32x4_t ym2 = vdupq_n_f32(ys[1]);
    float32x4_t ym1 = vdupq_n_f32(ys[0]);

    float32x4_t x = vdupq_laneq_f32(input, 0);
    float32x4_t xp1 = vdupq_laneq_f32(input, 1);
    float32x4_t xp2 = vdupq_laneq_f32(input, 2);
    float32x4_t xp3 = vdupq_laneq_f32(input, 3);

    float32x4_t y = vmulq_f32(coeffs_[0], xp3);
    y = vmlaq_f32(y, coeffs_[1], xp2);
    y = vmlaq_f32(y, coeffs_[2], xp1);
    y = vmlaq_f32(y, coeffs_[3], x);
    y = vmlaq_f32(y, coeffs_[4], xm1);
    y = vmlaq_f32(y, coeffs_[5], xm2);
    y = vmlaq_f32(y, coeffs_[6], ym1);
    y = vmlaq_f32(y, coeffs_[7], ym2);

    xs[0] = xp3[0];
    xs[1] = xp2[0];

    ys[0] = y[3];
    ys[1] = y[2];

    return y;
}

void BiquadNeonFilter::process(std::span<const float> input, std::span<float> output)
{
    assert(output.size() == input.size());

    auto simd_size = input.size() & ~3;

    float32x4_t xm2 = vdupq_n_f32(xs[1]);
    float32x4_t xm1 = vdupq_n_f32(xs[0]);
    float32x4_t ym2 = vdupq_n_f32(ys[1]);
    float32x4_t ym1 = vdupq_n_f32(ys[0]);

    if (simd_size > 0)
    {
        // update this here because input and output may point to the same buffer
        xs[0] = input[simd_size - 1];
        xs[1] = input[simd_size - 2];
    }

    for (size_t i = 0; i < simd_size; i += 4)
    {
        float32x4_t in = vld1q_f32(input.data() + i);
        float32x4_t x = vdupq_laneq_f32(in, 0);
        float32x4_t xp1 = vdupq_laneq_f32(in, 1);
        float32x4_t xp2 = vdupq_laneq_f32(in, 2);
        float32x4_t xp3 = vdupq_laneq_f32(in, 3);

        float32x4_t y = vmulq_f32(coeffs_[0], xp3);
        y = vmlaq_f32(y, coeffs_[1], xp2);
        y = vmlaq_f32(y, coeffs_[2], xp1);
        y = vmlaq_f32(y, coeffs_[3], x);
        y = vmlaq_f32(y, coeffs_[4], xm1);
        y = vmlaq_f32(y, coeffs_[5], xm2);
        y = vmlaq_f32(y, coeffs_[6], ym1);
        y = vmlaq_f32(y, coeffs_[7], ym2);

        xm2 = xp2;
        xm1 = xp3;
        ym2 = vdupq_n_f32(y[2]);
        ym1 = vdupq_n_f32(y[3]);

        vst1q_f32(output.data() + i, y);
    }

    if (simd_size > 0)
    {
        ys[0] = output[simd_size - 1];
        ys[1] = output[simd_size - 2];
    }

    for (size_t i = simd_size; i < input.size(); ++i)
    {
        output[i] = b0_ * input[i] + b1_ * xs[0] + b2_ * xs[1] - a1_ * ys[0] - a2_ * ys[1];
        xs[1] = xs[0];
        xs[0] = input[i];
        ys[1] = ys[0];
        ys[0] = output[i];
    }
}

NeonIIRFilter::NeonIIRFilter(size_t num_stage)
{
    size_t stage = num_stage == 0 ? kTestSOS.size() : num_stage;
    filters_.resize(stage);

    for (size_t i = 0; i < stage; i++)
    {
        float coeffs[5] = {kTestSOS[i % kTestSOS.size()][0] / kTestSOS[i % kTestSOS.size()][3],
                           kTestSOS[i % kTestSOS.size()][1] / kTestSOS[i % kTestSOS.size()][3],
                           kTestSOS[i % kTestSOS.size()][2] / kTestSOS[i % kTestSOS.size()][3],
                           kTestSOS[i % kTestSOS.size()][4] / kTestSOS[i % kTestSOS.size()][3],
                           kTestSOS[i % kTestSOS.size()][5] / kTestSOS[i % kTestSOS.size()][3]};
        filters_[i].SetCoeffs(coeffs);
    }
}

void NeonIIRFilter::process(std::span<const float> input, std::span<float> output)
{
    for (size_t i = 0; i < filters_.size(); ++i)
    {
        filters_[i].process(input, output);
        input = output;
    }
}