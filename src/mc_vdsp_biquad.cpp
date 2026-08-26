#include "mc_vdsp_biquad.h"

#include <stdexcept>

McVdspBiquad::~McVdspBiquad()
{
    for (auto setup : setups_)
    {
        if (setup != nullptr)
        {
            vDSP_biquad_DestroySetup(setup);
        }
    }
}

void McVdspBiquad::prepare(std::span<const std::array<float, 6>> sos, uint32_t channels, uint32_t stages)
{
    for (auto setup : setups_)
    {
        if (setup != nullptr)
        {
            vDSP_biquad_DestroySetup(setup);
        }
    }

    setups_.clear();
    delays_.clear();
    setups_.resize(channels, nullptr);
    delays_.resize(channels);

    for (uint32_t ch = 0; ch < channels; ++ch)
    {
        std::vector<double> coeffs(stages * 5, 0.0);

        for (uint32_t stage = 0; stage < stages; ++stage)
        {
            const auto& s = sos[stage + ch * stages];
            coeffs[stage * 5 + 0] = static_cast<double>(s[0] / s[3]);
            coeffs[stage * 5 + 1] = static_cast<double>(s[1] / s[3]);
            coeffs[stage * 5 + 2] = static_cast<double>(s[2] / s[3]);
            coeffs[stage * 5 + 3] = static_cast<double>(s[4] / s[3]);
            coeffs[stage * 5 + 4] = static_cast<double>(s[5] / s[3]);
        }

        delays_[ch].assign(stages * 2 + 2, 0.0f);
        setups_[ch] = vDSP_biquad_CreateSetup(coeffs.data(), stages);
        if (setups_[ch] == nullptr)
        {
            throw std::runtime_error("Failed to create vDSP biquad setup");
        }
    }
}

void McVdspBiquad::process(std::span<const float> input, std::span<float> output, uint32_t channels,
                          uint32_t frames)
{
    for (uint32_t ch = 0; ch < channels; ++ch)
    {
        vDSP_biquad(setups_[ch], delays_[ch].data(), input.data() + ch * frames, 1,
                    output.data() + ch * frames, 1, static_cast<vDSP_Length>(frames));
    }
}
