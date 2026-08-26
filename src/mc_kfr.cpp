#include "mc_kfr.h"

void McKfr::prepare(std::span<const std::array<float, 6>> sos, uint32_t channels, uint32_t stages)
{
    filters_.clear();
    filters_.resize(channels);
    coeffs_.assign(channels, {});
    stages_ = stages;

    for (uint32_t ch = 0; ch < channels; ++ch)
    {
        if (stages == 0)
        {
            filters_[ch].reset();
            continue;
        }

        std::vector<float>& coeffs = coeffs_[ch];
        coeffs.assign(stages * 6, 0.f);

        for (uint32_t stage = 0; stage < stages; ++stage)
        {
            const auto& section = sos[ch * stages + stage];
            // KFR's biquad_section field order is {a0, a1, a2, b0, b1, b2}; the numerator must NOT be negated.
            coeffs[stage * 6 + 0] = section[3];
            coeffs[stage * 6 + 1] = section[4];
            coeffs[stage * 6 + 2] = section[5];
            coeffs[stage * 6 + 3] = section[0];
            coeffs[stage * 6 + 4] = section[1];
            coeffs[stage * 6 + 5] = section[2];
        }

        filters_[ch] = std::make_unique<kfr::iir_filter<float>>(
            kfr::iir_params{ reinterpret_cast<const kfr::biquad_section<float>*>(coeffs.data()), stages });
    }
}

void McKfr::process(std::span<const float> input, std::span<float> output, uint32_t channels, uint32_t frames)
{
    if (channels == 0 || frames == 0 || stages_ == 0)
    {
        return;
    }

    for (uint32_t ch = 0; ch < channels; ++ch)
    {
        if (!filters_[ch])
        {
            continue;
        }

        // kfr::iir_filter carries its own filter state across apply() calls, which is what keeps
        // the cascade continuous when a block is processed in several pieces.
        const size_t offset = static_cast<size_t>(ch) * frames;
        filters_[ch]->apply(output.data() + offset, input.data() + offset, frames);
    }
}

const char* McKfr::name() const
{
    return "KFR_xN";
}
