#include "mc_scalar.h"

#include <stdexcept>

void McScalarDf2t::prepare(std::span<const std::array<float, 6>> sos, uint32_t channels,
                           uint32_t stages)
{
    if (channels == 0 || stages == 0)
    {
        channels_ = channels;
        stages_ = stages;
        coeffs_.clear();
        state_.clear();
        return;
    }

    const size_t section_count = static_cast<size_t>(channels) * static_cast<size_t>(stages);
    if (sos.size() != section_count)
    {
        throw std::invalid_argument("sos size does not match channels * stages");
    }

    channels_ = channels;
    stages_ = stages;
    coeffs_.resize(section_count);
    state_.assign(section_count * 2, 0.0f);

    for (uint32_t channel = 0; channel < channels; ++channel)
    {
        for (uint32_t stage = 0; stage < stages; ++stage)
        {
            const size_t index = static_cast<size_t>(channel) * stages + stage;
            const auto& section = sos[index];
            const float a0 = section[3];

            if (a0 == 0.0f)
            {
                throw std::invalid_argument("a0 must be non-zero");
            }

            coeffs_[index].b0 = section[0] / a0;
            coeffs_[index].b1 = section[1] / a0;
            coeffs_[index].b2 = section[2] / a0;
            coeffs_[index].a1 = section[4] / a0;
            coeffs_[index].a2 = section[5] / a0;
        }
    }
}

void McScalarDf2t::process(std::span<const float> input, std::span<float> output,
                          uint32_t channels, uint32_t frames)
{
    if (channels != channels_ || frames == 0)
    {
        return;
    }

    if (input.size() < static_cast<size_t>(channels) * frames || output.size() < static_cast<size_t>(channels) * frames)
    {
        throw std::invalid_argument("input/output buffer too small");
    }

    for (uint32_t channel = 0; channel < channels; ++channel)
    {
        const size_t base = static_cast<size_t>(channel) * frames;
        const size_t unroll_size = static_cast<size_t>(frames) & ~static_cast<size_t>(3);
        size_t sample = 0;

        while (sample < unroll_size)
        {
            size_t stage = 0;
            const float* in_ptr = &input[base + sample];
            float in1 = in_ptr[0];
            float in2 = in_ptr[1];
            float in3 = in_ptr[2];
            float in4 = in_ptr[3];
            float out1 = 0.0f;
            float out2 = 0.0f;
            float out3 = 0.0f;
            float out4 = 0.0f;

            while (stage < stages_)
            {
                const size_t section = static_cast<size_t>(channel) * stages_ + stage;
                const Coeffs& coeff = coeffs_[section];
                const float b0 = coeff.b0;
                const float b1 = coeff.b1;
                const float b2 = coeff.b2;
                const float a1 = coeff.a1;
                const float a2 = coeff.a2;
                float s0 = state_[section * 2];
                float s1 = state_[section * 2 + 1];

#define COMPUTE_SAMPLE(x, y)                                                                                           \
    y = b0 * x + s0;                                                                                                 \
    s0 = b1 * x + s1 - a1 * y;                                                                                        \
    s1 = b2 * x - a2 * y;

                COMPUTE_SAMPLE(in1, out1);
                COMPUTE_SAMPLE(in2, out2);
                COMPUTE_SAMPLE(in3, out3);
                COMPUTE_SAMPLE(in4, out4);

                in1 = out1;
                in2 = out2;
                in3 = out3;
                in4 = out4;

                state_[section * 2] = s0;
                state_[section * 2 + 1] = s1;

                ++stage;
            }

            output[base + sample] = out1;
            output[base + sample + 1] = out2;
            output[base + sample + 2] = out3;
            output[base + sample + 3] = out4;
            sample += 4;
        }

        while (sample < static_cast<size_t>(frames))
        {
            size_t stage = 0;
            float x = input[base + sample];
            float y = 0.0f;

            do
            {
                const size_t section = static_cast<size_t>(channel) * stages_ + stage;
                const Coeffs& coeff = coeffs_[section];
                float s0 = state_[section * 2];
                float s1 = state_[section * 2 + 1];

                y = coeff.b0 * x + s0;
                state_[section * 2] = coeff.b1 * x + s1 - coeff.a1 * y;
                state_[section * 2 + 1] = coeff.b2 * x - coeff.a2 * y;
                x = y;
                ++stage;
            } while (stage < stages_);

            output[base + sample] = x;
            ++sample;
        }
    }
}

const char* McScalarDf2t::name() const
{
    return "ScalarDF2T";
}
