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

BasicFilter::BasicFilter(size_t num_stage)
{
    size_t stage = num_stage == 0 ? kTestSOS.size() : num_stage;
    for (size_t i = 0; i < stage; i++)
    {
        std::vector<float> coeffs(5);
        coeffs[0] = kTestSOS[i % kTestSOS.size()][0] / kTestSOS[i % kTestSOS.size()][3];
        coeffs[1] = kTestSOS[i % kTestSOS.size()][1] / kTestSOS[i % kTestSOS.size()][3];
        coeffs[2] = kTestSOS[i % kTestSOS.size()][2] / kTestSOS[i % kTestSOS.size()][3];
        coeffs[3] = kTestSOS[i % kTestSOS.size()][4] / kTestSOS[i % kTestSOS.size()][3];
        coeffs[4] = kTestSOS[i % kTestSOS.size()][5] / kTestSOS[i % kTestSOS.size()][3];

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

CascadedIIRDF2T::CascadedIIRDF2T(size_t num_stage)
{
    stage_ = num_stage == 0 ? kTestSOS.size() : num_stage;
    coeffs_.resize(stage_ * 5);
    for (size_t i = 0; i < stage_; i++)
    {
        coeffs_[i].b0 = kTestSOS[i % kTestSOS.size()][0] / kTestSOS[i % kTestSOS.size()][3];
        coeffs_[i].b1 = kTestSOS[i % kTestSOS.size()][1] / kTestSOS[i % kTestSOS.size()][3];
        coeffs_[i].b2 = kTestSOS[i % kTestSOS.size()][2] / kTestSOS[i % kTestSOS.size()][3];
        coeffs_[i].a1 = kTestSOS[i % kTestSOS.size()][4] / kTestSOS[i % kTestSOS.size()][3];
        coeffs_[i].a2 = kTestSOS[i % kTestSOS.size()][5] / kTestSOS[i % kTestSOS.size()][3];
    }

    states_.resize(stage_, {0});
}

void CascadedIIRDF2T::process(std::span<const float> input, std::span<float> output)
{
    const float* in_ptr = input.data();
    float* out_ptr = output.data();

    size_t sample = 0;
    const size_t kSize = input.size();
    const size_t unroll_size = kSize & ~3;
    while (sample < unroll_size)
    {
        size_t stage = 0;
        float in1 = input[sample];
        float in2 = input[sample + 1];
        float in3 = input[sample + 2];
        float in4 = input[sample + 3];

        float out1 = 0;
        float out2 = 0;
        float out3 = 0;
        float out4 = 0;
        while (stage < stage_)
        {
            IIRCoeffs coeffs = coeffs_[stage];
            float s0 = states_[stage].s0;
            float s1 = states_[stage].s1;

#define COMPUTE_SAMPLE(x, y)                                                                                           \
    y = coeffs.b0 * x + s0;                                                                                            \
    s0 = coeffs.b1 * x + s1;                                                                                           \
    s0 -= coeffs.a1 * y;                                                                                               \
    s1 = coeffs.b2 * x;                                                                                                \
    s1 -= coeffs.a2 * y;

            COMPUTE_SAMPLE(in1, out1);
            COMPUTE_SAMPLE(in2, out2);
            COMPUTE_SAMPLE(in3, out3);
            COMPUTE_SAMPLE(in4, out4);

            in1 = out1;
            in2 = out2;
            in3 = out3;
            in4 = out4;

            states_[stage].s0 = s0;
            states_[stage].s1 = s1;

            ++stage;
        }

        output[sample] = out1;
        output[sample + 1] = out2;
        output[sample + 2] = out3;
        output[sample + 3] = out4;
        sample += 4;
    }

    while (sample < kSize)
    {
        size_t stage = 0;
        float in1 = input[sample];
        float out1 = 0;
        do
        {
            IIRCoeffs coeffs = coeffs_[stage];
            IIRState* state = &states_[stage];

            out1 = coeffs.b0 * in1 + state->s0;
            state->s0 = coeffs.b1 * in1 - coeffs.a1 * out1 + state->s1;
            state->s1 = coeffs.b2 * in1 - coeffs.a2 * out1;

            in1 = out1;
            ++stage;
        } while (stage < stage_);

        output[sample] = out1;
        ++sample;
    }
}

// void CascadedIIRDF2T::process(std::span<const float> input, std::span<float> output)
// {
//     const float* in_ptr = input.data();
//     float* out_ptr = output.data();
//     const size_t kSize = input.size();

//     size_t stage = 0;
//     do
//     {
//         IIRCoeffs coeffs = coeffs_[stage];
//         IIRState* state = &states_[stage];
//         float b0 = coeffs.b0;
//         float b1 = coeffs.b1;
//         float b2 = coeffs.b2;
//         float a1 = coeffs.a1;
//         float a2 = coeffs.a2;

//         float d1 = state->s0;
//         float d2 = state->s1;

//         size_t sample = 0;
//         const size_t unroll_size = kSize & ~3;
//         while (sample < kSize)
//         {
//             float in1 = in_ptr[sample];
//             float out = b0 * in1 + state->s0;
//             state->s0 = b1 * in1 - a1 * out + state->s1;
//             state->s1 = b2 * in1 - a2 * out;
//             out_ptr[sample] = out;

//             in1 = in_ptr[sample + 1];
//             out = b0 * in1 + state->s0;
//             state->s0 = b1 * in1 - a1 * out + state->s1;
//             state->s1 = b2 * in1 - a2 * out;
//             out_ptr[sample + 1] = out;

//             in1 = in_ptr[sample + 2];
//             out = b0 * in1 + state->s0;
//             state->s0 = b1 * in1 - a1 * out + state->s1;
//             state->s1 = b2 * in1 - a2 * out;
//             out_ptr[sample + 2] = out;

//             in1 = in_ptr[sample + 3];
//             out = b0 * in1 + state->s0;
//             state->s0 = b1 * in1 - a1 * out + state->s1;
//             state->s1 = b2 * in1 - a2 * out;
//             out_ptr[sample + 3] = out;

//             sample += 4;
//         }

//         while (sample < unroll_size)
//         {
//             float in1 = in_ptr[sample];
//             float out1 = coeffs.b0 * in1 + state->s0;
//             state->s0 = coeffs.b1 * in1 - coeffs.a1 * out1 + state->s1;
//             state->s1 = coeffs.b2 * in1 - coeffs.a2 * out1;

//             out_ptr[sample] = out1;
//             ++sample;
//         }

//         in_ptr = out_ptr;
//         ++stage;
//     } while (stage < stage_);
// }

CascadedIIRDF1::CascadedIIRDF1(size_t num_stage)
{
    stage_ = num_stage == 0 ? kTestSOS.size() : num_stage;
    coeffs_.resize(stage_ * 5);
    for (size_t i = 0; i < stage_; i++)
    {
        coeffs_[i].b0 = kTestSOS[i % kTestSOS.size()][0] / kTestSOS[i % kTestSOS.size()][3];
        coeffs_[i].b1 = kTestSOS[i % kTestSOS.size()][1] / kTestSOS[i % kTestSOS.size()][3];
        coeffs_[i].b2 = kTestSOS[i % kTestSOS.size()][2] / kTestSOS[i % kTestSOS.size()][3];
        coeffs_[i].a1 = kTestSOS[i % kTestSOS.size()][4] / kTestSOS[i % kTestSOS.size()][3];
        coeffs_[i].a2 = kTestSOS[i % kTestSOS.size()][5] / kTestSOS[i % kTestSOS.size()][3];
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
