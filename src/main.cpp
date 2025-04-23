#include <fstream>
#include <iostream>
#include <random>

#include <nanobench.h>

#include "filter.h"

#include "basic_filter.h"
#include "cmsis_scalar_filter.h"

#ifdef __APPLE__
#include "vdsp_filter.h"
#endif

using namespace ankerl;
using namespace std::chrono_literals;

template <typename T>
void RunFilter(std::span<const float> input, std::span<float> output)
{
    T filter;
    filter.process(input, output);
}

int main()
{
    constexpr std::array kBlockSize = {1, 4, 8, 16, 32, 64, 128, 256, 512, 1024};
    nanobench::Bench bench;
    bench.title("Filter Benchmark");
    bench.relative(true);
    bench.warmup(100);

    for (size_t i = 0; i < kBlockSize.size(); ++i)
    {
        size_t block_size = kBlockSize[i];
        std::vector<float> input(block_size, 0);

        bench.minEpochIterations((1024 / block_size) * 1000);
        // Fill with white noise
        std::default_random_engine generator;
        std::normal_distribution<double> dist(0, 0.1);
        for (size_t i = 0; i < input.size(); ++i)
        {
            input[i] = dist(generator);
        }

        std::vector<float> output(block_size, 0);

        // bench.timeUnit(1us, "us");
        bench.batch(block_size);
        bench.unit("samples");

        bench.run("CMSIS Scalar DF2T", [&]() { RunFilter<CMSISScalarFilterDF2T>(input, output); });
        bench.run("CMSIS Scalar DF1", [&]() { RunFilter<CMSISScalarFilterDF1>(input, output); });
        bench.run("Basic Filter", [&]() { RunFilter<BasicFilter>(input, output); });
        bench.run("Cascaded IIR DF2T", [&]() { RunFilter<CascadedIIRDF2T>(input, output); });
        bench.run("Cascaded IIR DF1", [&]() { RunFilter<CascadedIIRDF1>(input, output); });

#ifdef __APPLE__
        bench.run("vDSP Filter", [&]() { RunFilter<vDSPFilter>(input, output); });
#endif
    }
    std::ofstream render_out("perf_results.json");
    bench.render(ankerl::nanobench::templates::json(), render_out);

    std::ofstream html_out("perf_results.html");
    bench.render(ankerl::nanobench::templates::htmlBoxplot(), html_out);
}