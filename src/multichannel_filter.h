#pragma once

#include <array>
#include <cstdint>
#include <span>

/**
 * @brief Interface for filtering several channels through a cascade of biquads.
 *
 * The single-channel `Filter` interface in filter.h cannot represent implementations that
 * vectorize across the channel dimension, so multi-channel backends implement this instead.
 *
 * Audio layout is channel-major throughout: channel `c` occupies `[c * frames, (c + 1) * frames)`
 * in both the input and output spans.
 *
 * Second-order sections use this project's canonical `{b0, b1, b2, a0, a1, a2}` ordering (see
 * filter_coeffs.h) and are indexed `sos[channel * stages + stage]`, so every channel may have a
 * different filter.
 */
class MultiChannelFilter
{
  public:
    MultiChannelFilter() = default;
    virtual ~MultiChannelFilter() = default;

    MultiChannelFilter(const MultiChannelFilter&) = delete;
    MultiChannelFilter& operator=(const MultiChannelFilter&) = delete;
    MultiChannelFilter(MultiChannelFilter&&) = delete;
    MultiChannelFilter& operator=(MultiChannelFilter&&) = delete;

    /**
     * @brief Configures the filter bank.
     *
     * All allocation and coefficient-format conversion must happen here so that none of it is
     * attributed to process() during benchmarking.
     *
     * @param sos Sections indexed `[channel * stages + stage]`, each `{b0, b1, b2, a0, a1, a2}`.
     * @param channels Number of channels.
     * @param stages Number of biquad sections per channel.
     */
    virtual void prepare(std::span<const std::array<float, 6>> sos, uint32_t channels, uint32_t stages) = 0;

    /**
     * @brief Filters one block. Must be allocation-free.
     *
     * @param input Channel-major input, `channels * frames` samples.
     * @param output Channel-major output, `channels * frames` samples.
     * @param channels Must match the value passed to prepare().
     * @param frames Samples per channel. May differ between calls; filter state carries over.
     */
    virtual void process(std::span<const float> input, std::span<float> output, uint32_t channels,
                         uint32_t frames) = 0;

    /** @brief Short identifier used for benchmark names and result filenames. */
    virtual const char* name() const = 0;
};
