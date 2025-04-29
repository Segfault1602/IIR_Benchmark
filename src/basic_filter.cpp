#include "basic_filter.h"

#include "filter_coeffs.h"

BiquadFilter::BiquadFilter()
{
}

void BiquadFilter::SetCoeffs(std::span<const float> coeffs)
{
    b0_ = coeffs[0];
    b1_ = coeffs[1];
    b2_ = coeffs[2];
    a1_ = coeffs[3];
    a2_ = coeffs[4];
    x0_ = 0.f;
    x1_ = 0.f;
    x2_ = 0.f;
    y0_ = 0.f;
    y1_ = 0.f;
    y2_ = 0.f;
}

float BiquadFilter::process(float input)
{

    x0_ = input;
    y0_ = b0_ * x0_ + b1_ * x1_ + b2_ * x2_ - a1_ * y1_ - a2_ * y2_;

    y2_ = y1_;
    y1_ = y0_;
    x2_ = x1_;
    x1_ = x0_;
    return y0_;
}

BasicFilter::BasicFilter()
{
    for (size_t i = 0; i < kTestSOS.size(); i++)
    {
        std::vector<float> coeffs(5);
        coeffs[0] = kTestSOS[i][0] / kTestSOS[i][3];
        coeffs[1] = kTestSOS[i][1] / kTestSOS[i][3];
        coeffs[2] = kTestSOS[i][2] / kTestSOS[i][3];
        coeffs[3] = kTestSOS[i][4] / kTestSOS[i][3];
        coeffs[4] = kTestSOS[i][5] / kTestSOS[i][3];

        BiquadFilter filter;
        filter.SetCoeffs(coeffs);
        filters_.push_back(filter);
    }
}

void BasicFilter::process(std::span<const float> input, std::span<float> output)
{
    for (size_t i = 0; i < input.size(); ++i)
    {
        float y = input[i];
        for (size_t i = 0; i < filters_.size(); ++i)
        {
            y = filters_[i].process(y);
        }
        output[i] = y;
    }
}

CascadedIIRDF2T::CascadedIIRDF2T()
{
    stage_ = kTestSOS.size();
    coeffs_.resize(stage_ * 5);
    for (size_t i = 0; i < kTestSOS.size(); i++)
    {
        coeffs_[i].b0 = kTestSOS[i][0] / kTestSOS[i][3];
        coeffs_[i].b1 = kTestSOS[i][1] / kTestSOS[i][3];
        coeffs_[i].b2 = kTestSOS[i][2] / kTestSOS[i][3];
        coeffs_[i].a1 = kTestSOS[i][4] / kTestSOS[i][3];
        coeffs_[i].a2 = kTestSOS[i][5] / kTestSOS[i][3];
    }

    states_.resize(stage_, {0});
}

void CascadedIIRDF2T::process(std::span<const float> input, std::span<float> output)
{
    const float* in_ptr = input.data();
    float* out_ptr = output.data();

    size_t sample = 0;
    while (sample < input.size())
    {
        size_t stage = 0;
        float in1 = input[sample];
        float out1 = 0;
        while (stage < stage_)
        {
            IIRCoeffs coeffs = coeffs_[stage];
            IIRState* state = &states_[stage];

            out1 = coeffs.b0 * in1 + state->s0;
            state->s0 = coeffs.b1 * in1 - coeffs.a1 * out1 + state->s1;
            state->s1 = coeffs.b2 * in1 - coeffs.a2 * out1;

            in1 = out1;
            ++stage;
        }

        output[sample] = out1;
        ++sample;
    }
}

CascadedIIRDF1::CascadedIIRDF1()
{
    stage_ = kTestSOS.size();
    coeffs_.resize(stage_ * 5);
    for (size_t i = 0; i < kTestSOS.size(); i++)
    {
        coeffs_[i].b0 = kTestSOS[i][0] / kTestSOS[i][3];
        coeffs_[i].b1 = kTestSOS[i][1] / kTestSOS[i][3];
        coeffs_[i].b2 = kTestSOS[i][2] / kTestSOS[i][3];
        coeffs_[i].a1 = kTestSOS[i][4] / kTestSOS[i][3];
        coeffs_[i].a2 = kTestSOS[i][5] / kTestSOS[i][3];
    }

    states_.resize(stage_, {0});
}

void CascadedIIRDF1::process(std::span<const float> input, std::span<float> output)
{
    const float* in_ptr = input.data();
    float* out_ptr = output.data();

    size_t sample = input.size();
    while (sample > 0)
    {
        size_t stage = 0;
        float in1 = *in_ptr++;
        float out1 = 0;
        do
        {
            IIRCoeffs coeffs = coeffs_[stage];
            IIRState* state = &states_[stage];

            out1 = coeffs.b0 * in1 + coeffs.b1 * state->x1 + coeffs.b2 * state->x2 - coeffs.a1 * state->y1 -
                   coeffs.a2 * state->y2;

            state->y2 = state->y1;
            state->y1 = out1;
            state->x2 = state->x1;
            state->x1 = in1;
            in1 = out1;

            ++stage;
        } while (stage < stage_);

        *out_ptr++ = in1;
        --sample;
    }
}
