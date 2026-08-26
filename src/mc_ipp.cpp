#include "mc_ipp.h"

#include <iostream>

void McIpp::prepare(std::span<const std::array<float, 6>> sos, uint32_t channels, uint32_t stages)
{
    channels_.clear();
    channels_.resize(channels);

    if (stages == 0)
    {
        return;
    }

    const int num_bq = static_cast<int>(stages);

    int buffer_size = 0;
    IppStatus status = ippsIIRGetStateSize_BiQuad_32f(num_bq, &buffer_size);
    if (status != ippStsNoErr)
    {
        std::cerr << "McIpp: ippsIIRGetStateSize_BiQuad_32f failed: " << status << std::endl;
        return;
    }

    for (uint32_t ch = 0; ch < channels; ++ch)
    {
        ChannelState& channel = channels_[ch];
        channel.buffer.reset(ippsMalloc_8u(buffer_size));
        channel.taps.reset(ippsMalloc_32f(6 * num_bq));
        channel.delay_line.reset(ippsMalloc_32f(2 * num_bq));

        if (channel.buffer == nullptr || channel.taps == nullptr || channel.delay_line == nullptr)
        {
            std::cerr << "McIpp: allocation failed" << std::endl;
            return;
        }

        for (uint32_t stage = 0; stage < stages; ++stage)
        {
            const auto& section = sos[static_cast<size_t>(ch) * stages + stage];
            for (size_t coeff = 0; coeff < section.size(); ++coeff)
            {
                channel.taps.get()[stage * 6 + coeff] = section[coeff];
            }
        }

        std::fill_n(channel.delay_line.get(), 2 * num_bq, 0.f);

        status = ippsIIRInit_BiQuad_32f(&channel.state, channel.taps.get(), num_bq,
                                        channel.delay_line.get(), channel.buffer.get());
        if (status != ippStsNoErr)
        {
            std::cerr << "McIpp: ippsIIRInit_BiQuad_32f failed: " << status << std::endl;
            channel.state = nullptr;
            return;
        }
    }
}

void McIpp::process(std::span<const float> input, std::span<float> output, uint32_t channels,
                    uint32_t frames)
{
    if (channels == 0 || frames == 0 || channels_.size() != channels)
    {
        return;
    }

    for (uint32_t ch = 0; ch < channels; ++ch)
    {
        IppsIIRState_32f* state = channels_[ch].state;
        if (state == nullptr)
        {
            continue;
        }

        const float* in_ptr = input.data() + static_cast<size_t>(ch) * frames;
        float* out_ptr = output.data() + static_cast<size_t>(ch) * frames;
        ippsIIR_32f(in_ptr, out_ptr, static_cast<int>(frames), state);
    }
}

const char* McIpp::name() const
{
    return "IPP_xN";
}
