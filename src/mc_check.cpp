#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "multichannel_filter.h"

#include "mc_cmsis.h"
#include "mc_kfr.h"
#include "mc_scalar.h"
#include "mc_simd_bank.h"
#include "mc_steamaudio.h"

#ifdef MC_HAS_VDSP
#include "mc_vdsp_biquad.h"
#include "mc_vdsp_biquadm.h"
#endif

#include <array>
#include <cmath>
#include <cstdint>
#include <memory>
#include <random>
#include <string>
#include <vector>

namespace
{
constexpr uint32_t kFrames = 200;

// Deliberately uneven, and deliberately not aligned to the SIMD bank's internal 64-sample chunk,
// so that filter state has to survive partial chunks and arbitrary call boundaries. Both patterns
// sum to kFrames.
//
// Two patterns are used because KFR cannot handle very small blocks: kfr::iir_filter::apply()
// corrupts its state when a call of 2 to 15 samples is followed by more data (measured; calls of
// 16 or more are correct). The benchmark only ever uses blocks of 32 to 512 samples, so this does
// not affect any measurement, but it does mean KFR has to sit out the small-block pattern.
constexpr std::array<uint32_t, 5> kChunks = {37, 16, 64, 22, 61};
constexpr std::array<uint32_t, 5> kSmallChunks = {37, 5, 64, 22, 72};

std::vector<std::unique_ptr<MultiChannelFilter>> MakeBackends()
{
    std::vector<std::unique_ptr<MultiChannelFilter>> backends;
    backends.push_back(std::make_unique<McScalarDf2t>());
    backends.push_back(std::make_unique<McSimdBank>());
    backends.push_back(std::make_unique<McKfr>());
    backends.push_back(std::make_unique<McCmsis>());
    backends.push_back(std::make_unique<McSteamAudio>());
#ifdef MC_HAS_VDSP
    backends.push_back(std::make_unique<McVdspBiquad>());
    backends.push_back(std::make_unique<McVdspBiquadm>());
#endif
    return backends;
}

/// Builds a stable cascade per channel, with every channel getting DIFFERENT coefficients so that
/// a channel or SIMD-lane mix-up cannot pass this test.
std::vector<std::array<float, 6>> MakeCoefficients(uint32_t channels, uint32_t stages, uint32_t seed)
{
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> b0_dist(0.4f, 0.8f);
    std::uniform_real_distribution<float> b_dist(-0.3f, 0.3f);
    std::uniform_real_distribution<float> a_dist(-0.2f, 0.2f);

    std::vector<std::array<float, 6>> sos(static_cast<size_t>(channels) * stages);
    for (auto& section : sos)
    {
        section = {b0_dist(rng), b_dist(rng), b_dist(rng), 1.0f, a_dist(rng), a_dist(rng)};
    }
    return sos;
}

/// Transposed direct form II in double precision, one channel at a time. This is the reference
/// every backend is measured against.
std::vector<double> Reference(std::span<const std::array<float, 6>> sos, std::span<const float> input,
                              uint32_t channels, uint32_t stages, uint32_t frames)
{
    std::vector<double> output(input.begin(), input.end());

    for (uint32_t channel = 0; channel < channels; ++channel)
    {
        for (uint32_t stage = 0; stage < stages; ++stage)
        {
            const auto& c = sos[(static_cast<size_t>(channel) * stages) + stage];
            const double b0 = c[0] / c[3];
            const double b1 = c[1] / c[3];
            const double b2 = c[2] / c[3];
            const double a1 = c[4] / c[3];
            const double a2 = c[5] / c[3];

            double s0 = 0.0;
            double s1 = 0.0;
            for (uint32_t n = 0; n < frames; ++n)
            {
                double& sample = output[(static_cast<size_t>(channel) * frames) + n];
                const double x = sample;
                const double y = (b0 * x) + s0;
                s0 = (b1 * x) + s1 - (a1 * y);
                s1 = (b2 * x) - (a2 * y);
                sample = y;
            }
        }
    }
    return output;
}

double MaxError(std::span<const float> actual, std::span<const double> expected)
{
    double worst = 0.0;
    for (size_t i = 0; i < expected.size(); ++i)
    {
        worst = std::max(worst, std::abs(static_cast<double>(actual[i]) - expected[i]));
    }
    return worst;
}

std::vector<float> MakeNoise(size_t count, uint32_t seed)
{
    std::mt19937 rng(seed);
    std::normal_distribution<float> dist(0.f, 0.25f);
    std::vector<float> noise(count);
    for (auto& sample : noise)
    {
        sample = dist(rng);
    }
    return noise;
}
} // namespace

TEST_CASE("multi-channel backends match a double precision reference")
{
    // 17 and 33 are not multiples of the SIMD width, so they exercise the bank's padding lanes.
    for (uint32_t channels : {1u, 2u, 3u, 4u, 5u, 8u, 16u, 17u, 32u, 33u})
    {
        // 12 is what the benchmark measures; 11 is kept deliberately because a numerator sign
        // error produces a (-1)^N output flip, which is invisible at even section counts. That
        // is exactly how the KFR sign bug hid in the pre-existing single-channel wrapper.
        for (uint32_t stages : {2u, 11u, 12u})
        {
            const auto sos = MakeCoefficients(channels, stages, (channels * 100) + stages);
            const auto input = MakeNoise(static_cast<size_t>(channels) * kFrames, 1234);
            const auto expected = Reference(sos, input, channels, stages, kFrames);

            for (auto& backend : MakeBackends())
            {
                const std::string backend_name = backend->name();
                CAPTURE(backend_name);
                CAPTURE(channels);
                CAPTURE(stages);

                std::vector<float> output(input.size(), 0.f);
                backend->prepare(sos, channels, stages);
                backend->process(input, output, channels, kFrames);

                CHECK(MaxError(output, expected) < 1e-4);
            }
        }
    }
}

TEST_CASE("multi-channel backends preserve state across uneven blocks")
{
    // Processing the same signal in one call and in ragged pieces must give the same answer, which
    // is what catches filter state being reset or misindexed between calls.
    for (uint32_t channels : {1u, 4u, 8u, 17u, 32u})
    {
        constexpr uint32_t kStages = 11;
        const auto sos = MakeCoefficients(channels, kStages, channels + 7);
        const auto input = MakeNoise(static_cast<size_t>(channels) * kFrames, 99);
        const auto expected = Reference(sos, input, channels, kStages, kFrames);

        for (auto& backend : MakeBackends())
        {
            const std::string backend_name = backend->name();
                CAPTURE(backend_name);
            CAPTURE(channels);

            // KFR is exercised only with the >= 16 sample pattern, for the reason documented at
            // kChunks above.
            const bool supports_small_blocks = backend_name != "KFR_xN";
            const std::span<const uint32_t> chunks =
                supports_small_blocks ? std::span<const uint32_t>(kSmallChunks) : std::span<const uint32_t>(kChunks);

            std::vector<float> output(input.size(), 0.f);
            backend->prepare(sos, channels, kStages);

            // Each chunk is a channel-major sub-block, so it must be gathered per channel rather
            // than taken as one contiguous slice of the whole buffer.
            uint32_t offset = 0;
            for (uint32_t size : chunks)
            {
                std::vector<float> chunk_in(static_cast<size_t>(channels) * size);
                std::vector<float> chunk_out(static_cast<size_t>(channels) * size, 0.f);
                for (uint32_t channel = 0; channel < channels; ++channel)
                {
                    const float* src = input.data() + (static_cast<size_t>(channel) * kFrames) + offset;
                    std::copy_n(src, size, chunk_in.begin() + (static_cast<size_t>(channel) * size));
                }

                backend->process(chunk_in, chunk_out, channels, size);

                for (uint32_t channel = 0; channel < channels; ++channel)
                {
                    float* dst = output.data() + (static_cast<size_t>(channel) * kFrames) + offset;
                    std::copy_n(chunk_out.data() + (static_cast<size_t>(channel) * size), size, dst);
                }
                offset += size;
            }
            REQUIRE(offset == kFrames);

            CHECK(MaxError(output, expected) < 1e-4);
        }
    }
}

TEST_CASE("multi-channel backends keep channels independent")
{
    // Silence every channel but one. Any leakage into the silent channels means channels are being
    // mixed, which a uniform-input test would not reveal.
    constexpr uint32_t kChannels = 8;
    constexpr uint32_t kStages = 4;

    const auto sos = MakeCoefficients(kChannels, kStages, 4242);

    for (uint32_t live = 0; live < kChannels; live += 3)
    {
        std::vector<float> input(static_cast<size_t>(kChannels) * kFrames, 0.f);
        const auto noise = MakeNoise(kFrames, 55);
        std::copy(noise.begin(), noise.end(), input.begin() + (static_cast<size_t>(live) * kFrames));

        for (auto& backend : MakeBackends())
        {
            const std::string backend_name = backend->name();
                CAPTURE(backend_name);
            CAPTURE(live);

            std::vector<float> output(input.size(), 0.f);
            backend->prepare(sos, kChannels, kStages);
            backend->process(input, output, kChannels, kFrames);

            for (uint32_t channel = 0; channel < kChannels; ++channel)
            {
                if (channel == live)
                {
                    continue;
                }
                double leaked = 0.0;
                for (uint32_t n = 0; n < kFrames; ++n)
                {
                    leaked = std::max(leaked,
                                      std::abs(static_cast<double>(output[(static_cast<size_t>(channel) * kFrames) + n])));
                }
                CAPTURE(channel);
                // Not exactly zero: SteamAudio deliberately adds a 1e-9 offset to its input as
                // denormal protection (externals/steamaudio_iir/iir.cpp:127 and :166-171), so a
                // silent channel legitimately produces a tiny non-zero output. Genuine channel
                // mixing would leak at the order of the signal itself (~0.25 here), so this
                // threshold still catches it by a wide margin.
                CHECK(leaked < 1e-6);
            }
        }
    }
}
