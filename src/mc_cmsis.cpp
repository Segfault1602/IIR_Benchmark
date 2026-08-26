#include "mc_cmsis.h"

void McCmsis::prepare(std::span<const std::array<float, 6>> sos, uint32_t channels, uint32_t stages)
{
    instances_.clear();
    coeffs_.clear();
    states_.clear();

    instances_.resize(channels);
    coeffs_.resize(channels);
    states_.resize(channels);
    stages_ = stages;

    for (uint32_t ch = 0; ch < channels; ++ch)
    {
        coeffs_[ch].resize(stages * 5);
        states_[ch].resize(stages * 8, 0.0f);

        for (uint32_t stage = 0; stage < stages; ++stage)
        {
            const auto& section = sos[ch * stages + stage];
            coeffs_[ch][stage * 5 + 0] = section[0] / section[3];
            coeffs_[ch][stage * 5 + 1] = section[1] / section[3];
            coeffs_[ch][stage * 5 + 2] = section[2] / section[3];
            coeffs_[ch][stage * 5 + 3] = -section[4] / section[3];
            coeffs_[ch][stage * 5 + 4] = -section[5] / section[3];
        }

#if defined(ARM_MATH_NEON)
        std::vector<float> computed(stages * 8);
        arm_biquad_cascade_df2T_compute_coefs_f32(stages, coeffs_[ch].data(), computed.data());
        coeffs_[ch] = computed;
#endif

        arm_biquad_cascade_df2T_init_f32(&instances_[ch], stages, coeffs_[ch].data(), states_[ch].data());
    }
}

void McCmsis::process(std::span<const float> input, std::span<float> output, uint32_t channels, uint32_t frames)
{
    if (channels == 0 || frames == 0 || stages_ == 0)
    {
        return;
    }

    for (uint32_t ch = 0; ch < channels; ++ch)
    {
        arm_biquad_cascade_df2T_f32(&instances_[ch],
                                    input.data() + static_cast<size_t>(ch) * frames,
                                    output.data() + static_cast<size_t>(ch) * frames,
                                    frames);
    }
}

const char* McCmsis::name() const
{
    return "CMSIS_DF2T_xN";
}
