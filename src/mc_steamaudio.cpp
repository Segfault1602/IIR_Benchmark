#include "mc_steamaudio.h"

#include "steamaudio_kernel.h"

#include <string>

void McSteamAudio::prepare(std::span<const std::array<float, 6>> sos, uint32_t channels,
                          uint32_t stages)
{
    filterers_.clear();
    filterers_.resize(channels);
    stages_ = stages;

    for (uint32_t ch = 0; ch < channels; ++ch)
    {
        filterers_[ch].resize(stages);

        for (uint32_t stage = 0; stage < stages; ++stage)
        {
            const auto& section = sos[static_cast<size_t>(ch) * stages + stage];
            ipl::IIR2 iir{};
            iir.b0 = section[0] / section[3];
            iir.b1 = section[1] / section[3];
            iir.b2 = section[2] / section[3];
            iir.a1 = section[4] / section[3];
            iir.a2 = section[5] / section[3];
            filterers_[ch][stage].setFilter(iir);
            filterers_[ch][stage].reset();
        }
    }
}

void McSteamAudio::process(std::span<const float> input, std::span<float> output, uint32_t channels,
                          uint32_t frames)
{
    if (channels == 0 || frames == 0 || stages_ == 0)
    {
        return;
    }

    for (uint32_t ch = 0; ch < channels; ++ch)
    {
        const float* in_ptr = input.data() + static_cast<size_t>(ch) * frames;
        float* out_ptr = output.data() + static_cast<size_t>(ch) * frames;

        for (uint32_t stage = 0; stage < stages_; ++stage)
        {
            filterers_[ch][stage].apply(frames, in_ptr, out_ptr);
            in_ptr = out_ptr;
        }
    }
}

const char* McSteamAudio::name() const
{
    // SteamAudio chooses its kernel width at run time, so resolve the label once on first use.
    static const std::string kName = std::string("SteamAudio_") + SteamAudioKernelName() + "_xN";
    return kName.c_str();
}
