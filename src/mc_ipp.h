#pragma once

#include "multichannel_filter.h"

#include <array>
#include <ipp.h>
#include <memory>
#include <span>
#include <vector>

/**
 * @brief Intel IPP biquad cascade instantiated once per channel.
 *
 * IPP has no multi-channel biquad entry point, so like the other `_xN` backends this filters each
 * channel independently with `ippsIIR_32f`. Every channel owns its own state, taps and delay line
 * because a single IPP state carries the delay line for one signal only.
 */
class McIpp : public MultiChannelFilter
{
  public:
    McIpp() = default;
    ~McIpp() override = default;

    void prepare(std::span<const std::array<float, 6>> sos, uint32_t channels, uint32_t stages) override;
    void process(std::span<const float> input, std::span<float> output, uint32_t channels,
                 uint32_t frames) override;
    const char* name() const override;

  private:
    struct IppFree
    {
        void operator()(void* p) const noexcept
        {
            ippsFree(p);
        }
    };

    /// IPP requires its own aligned allocations, and the delay line and taps must outlive the state.
    struct ChannelState
    {
        std::unique_ptr<Ipp8u, IppFree> buffer;
        std::unique_ptr<Ipp32f, IppFree> taps;
        std::unique_ptr<Ipp32f, IppFree> delay_line;
        IppsIIRState_32f* state = nullptr;
    };

    std::vector<ChannelState> channels_;
};
