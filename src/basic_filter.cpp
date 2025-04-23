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

void BiquadFilter::process(std::span<const float> input, std::span<float> output)
{
    for (size_t i = 0; i < input.size(); ++i)
    {
        x0_ = input[i];
        y0_ = b0_ * x0_ + b1_ * x1_ + b2_ * x2_ - a1_ * y1_ - a2_ * y2_;

        y2_ = y1_;
        y1_ = y0_;
        x2_ = x1_;
        x1_ = x0_;
        output[i] = y0_;
    }
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
    for (size_t i = 0; i < filters_.size(); ++i)
    {
        filters_[i].process(input, output);
        input = output;
    }
}

CascadedIIRDF2T::CascadedIIRDF2T()
{
    stage_ = kTestSOS.size();
    coeffs_.resize(stage_ * 5);
    for (size_t i = 0; i < kTestSOS.size(); i++)
    {
        coeffs_[i * 5 + 0] = kTestSOS[i][0] / kTestSOS[i][3];
        coeffs_[i * 5 + 1] = kTestSOS[i][1] / kTestSOS[i][3];
        coeffs_[i * 5 + 2] = kTestSOS[i][2] / kTestSOS[i][3];
        coeffs_[i * 5 + 3] = kTestSOS[i][4] / kTestSOS[i][3];
        coeffs_[i * 5 + 4] = kTestSOS[i][5] / kTestSOS[i][3];
    }

    states_.resize(stage_ * 2, 0);
}

void CascadedIIRDF2T::process(std::span<const float> input, std::span<float> output)
{
    const float* in_ptr = input.data();
    float* out_ptr = output.data();

    size_t sample = input.size();
    while (sample > 0)
    {
        float* coeffs_ptr = coeffs_.data();
        float* state_ptr = states_.data();
        size_t stage = stage_;
        float in1 = *in_ptr++;
        float out1 = 0;
        do
        {
            float* b = coeffs_ptr;
            coeffs_ptr += 3;
            float* a = coeffs_ptr;
            coeffs_ptr += 2;

            float* state = state_ptr;
            state_ptr += 2;

            out1 = b[0] * in1 + state[0];
            state[0] = b[1] * in1 - a[0] * out1 + state[1];
            state[1] = b[2] * in1 - a[1] * out1;

            in1 = out1;
            --stage;
        } while (stage > 0);

        *out_ptr++ = out1;
        --sample;
    }
}

CascadedIIRDF1::CascadedIIRDF1()
{
    stage_ = kTestSOS.size();
    coeffs_.resize(stage_ * 5);
    for (size_t i = 0; i < kTestSOS.size(); i++)
    {
        coeffs_[i * 5 + 0] = kTestSOS[i][0] / kTestSOS[i][3];
        coeffs_[i * 5 + 1] = kTestSOS[i][1] / kTestSOS[i][3];
        coeffs_[i * 5 + 2] = kTestSOS[i][2] / kTestSOS[i][3];
        coeffs_[i * 5 + 3] = kTestSOS[i][4] / kTestSOS[i][3];
        coeffs_[i * 5 + 4] = kTestSOS[i][5] / kTestSOS[i][3];
    }

    states_.resize(stage_ * 5, 0);
}

void CascadedIIRDF1::process(std::span<const float> input, std::span<float> output)
{
    const float* in_ptr = input.data();
    float* out_ptr = output.data();

    size_t sample = input.size();
    while (sample > 0)
    {
        float* coeffs_ptr = coeffs_.data();
        float* state_ptr = states_.data();
        size_t stage = stage_;
        float in1 = *in_ptr++;
        float out1 = 0;
        do
        {
            float* b = coeffs_ptr;
            coeffs_ptr += 3;
            float* a = coeffs_ptr;
            coeffs_ptr += 2;

            float* state = state_ptr;
            state_ptr += 5;

            state[0] = in1;
            out1 = b[0] * state[0] + b[1] * state[1] + b[2] * state[2] - a[0] * state[3] - a[1] * state[4];

            state[4] = state[3];
            state[3] = out1;
            state[2] = state[1];
            state[1] = state[0];
            in1 = out1;

            --stage;
        } while (stage > 0);

        *out_ptr++ = in1;
        --sample;
    }
}
