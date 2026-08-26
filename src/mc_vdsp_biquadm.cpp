#include "mc_vdsp_biquadm.h"

#include <stdexcept>

McVdspBiquadm::~McVdspBiquadm()
{
    if (setup_ != nullptr)
    {
        vDSP_biquadm_DestroySetup(setup_);
    }
}

void McVdspBiquadm::prepare(std::span<const std::array<float, 6>> sos, uint32_t channels,
                           uint32_t stages)
{
    if (setup_ != nullptr)
    {
        vDSP_biquadm_DestroySetup(setup_);
        setup_ = nullptr;
    }

    std::vector<double> coeffs;
    coeffs.reserve(channels * stages * 5);

    for (uint32_t stage = 0; stage < stages; ++stage)
    {
        for (uint32_t ch = 0; ch < channels; ++ch)
        {
            const auto& s = sos[stage + ch * stages];
            coeffs.push_back(static_cast<double>(s[0] / s[3]));
            coeffs.push_back(static_cast<double>(s[1] / s[3]));
            coeffs.push_back(static_cast<double>(s[2] / s[3]));
            coeffs.push_back(static_cast<double>(s[4] / s[3]));
            coeffs.push_back(static_cast<double>(s[5] / s[3]));
        }
    }

    setup_ = vDSP_biquadm_CreateSetup(coeffs.data(), stages, channels);
    if (setup_ == nullptr)
    {
        throw std::runtime_error("Failed to create vDSP biquad setup");
    }

    vDSP_biquadm_SetCoefficientsDouble(setup_, coeffs.data(), 0, 0, stages, channels);

    input_ptrs_.resize(channels);
    output_ptrs_.resize(channels);
}

void McVdspBiquadm::process(std::span<const float> input, std::span<float> output, uint32_t channels,
                           uint32_t frames)
{
    for (uint32_t ch = 0; ch < channels; ++ch)
    {
        input_ptrs_[ch] = input.data() + ch * frames;
        output_ptrs_[ch] = output.data() + ch * frames;
    }

    vDSP_biquadm(setup_, input_ptrs_.data(), 1, output_ptrs_.data(), 1,
                 static_cast<vDSP_Length>(frames));
}
