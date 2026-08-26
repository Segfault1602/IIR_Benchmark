#include "filter_coeffs.h"
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

#include <nanobench.h>

#include <array>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <span>
#include <string>
#include <utility>
#include <vector>

using namespace std::chrono_literals;

namespace
{
constexpr std::array<uint32_t, 9> kChannels = {1u, 2u, 4u, 8u, 16u, 17u, 32u, 33u, 64u};
// 12 matches the single-channel benchmark, whose kTestSOS cascade has 12 sections, so the two
// sets of results are directly comparable.
constexpr std::array<uint32_t, 2> kStages = {2u, 12u};
constexpr std::array<uint32_t, 4> kBlocks = {32u, 64u, 128u, 512u};

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

std::vector<std::array<float, 6>> MakeSos(uint32_t channels, uint32_t stages)
{
    std::vector<std::array<float, 6>> sos(static_cast<size_t>(channels) * stages);
    for (uint32_t channel = 0; channel < channels; ++channel)
    {
        for (uint32_t stage = 0; stage < stages; ++stage)
        {
            const size_t index = static_cast<size_t>(channel) * stages + stage;
            sos[index] = kTestSOS[(channel + stage) % kTestSOS.size()];
        }
    }
    return sos;
}

std::vector<float> MakeNoise(size_t count)
{
    std::vector<float> noise(count);
    std::default_random_engine generator;
    std::normal_distribution<double> dist(0.0, 0.1);
    for (float& sample : noise)
    {
        sample = static_cast<float>(dist(generator));
    }
    return noise;
}

void PrintSummary(uint32_t stages, uint32_t block, std::vector<std::pair<std::string, std::vector<double>>> const& rows,
                  std::vector<std::string> const& backend_names)
{
    std::cout << "\n=== stages=" << stages << " block=" << block << " ===\n";
    std::cout << std::left << std::setw(9) << "ch";
    for (const auto& name : backend_names)
    {
        std::cout << std::setw(16) << name;
    }
    std::cout << "\n";

    for (auto const& row : rows)
    {
        std::cout << std::setw(9) << row.first;
        for (double ns_per_sample : row.second)
        {
            std::cout << std::fixed << std::setprecision(3) << std::setw(16) << ns_per_sample;
        }
        std::cout << "\n";
    }
}

} // namespace

int main()
{
    std::cout << "Running multi-channel biquad benchmarks..." << std::endl;

    for (uint32_t stages : kStages)
    {
        for (uint32_t block : kBlocks)
        {
            auto backends = MakeBackends();
            std::vector<std::string> backend_names;
            backend_names.reserve(backends.size());
            for (const auto& backend : backends)
            {
                backend_names.emplace_back(backend->name());
            }

            ankerl::nanobench::Bench bench;
            bench.title("stages=" + std::to_string(stages) + " block=" + std::to_string(block));
            bench.relative(true);
            bench.warmup(100);
            bench.unit("samples");
            bench.minEpochTime(10ms);

            std::vector<std::pair<std::string, std::vector<double>>> rows;
            rows.reserve(kChannels.size());

            for (uint32_t channels : kChannels)
            {
                const auto sos = MakeSos(channels, stages);
                const auto input = MakeNoise(static_cast<size_t>(channels) * block);
                std::vector<float> output(static_cast<size_t>(channels) * block, 0.0f);

                std::vector<double> channel_times;
                channel_times.reserve(backends.size());

                for (const auto& backend : backends)
                {
                    backend->prepare(sos, channels, stages);
                    bench.batch(static_cast<int>(channels * block));
                    const std::string test_name = std::string(backend->name()) + "_ch" + std::to_string(channels);
                    bench.run(test_name, [&]() { backend->process(input, output, channels, block); });

                    const auto& result = bench.results().back();
                    const double elapsed_seconds = result.median(ankerl::nanobench::Result::Measure::elapsed);
                    const double ns_per_sample = (elapsed_seconds * 1e9) / static_cast<double>(channels * block);
                    channel_times.push_back(ns_per_sample);
                }

                rows.emplace_back(std::to_string(channels), std::move(channel_times));
            }

            std::cout << "\nBenchmarking stages=" << stages << " block=" << block << "..." << std::endl;
            PrintSummary(stages, block, rows, backend_names);

            const std::string filename =
                "perf_results_mc_st" + std::to_string(stages) + "_blk" + std::to_string(block) + ".json";
            std::ofstream out(filename);
            bench.render(ankerl::nanobench::templates::json(), out);
            std::cout << "Wrote " << filename << std::endl;
        }
    }

    std::cout << "All multi-channel tests completed." << std::endl;
    return 0;
}
