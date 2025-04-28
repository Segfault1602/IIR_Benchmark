#include <cassert>
#include <fstream>
#include <iostream>
#include <random>

#include <nanobench.h>

#include "filter.h"

#include "basic_filter.h"

#if defined(CMSIS_FILTER_SCALAR) || defined(CMSIS_FILTER_NEON)
#include "cmsis_filter.h"
#endif

#ifdef VDSP_FILTER
#include "vdsp_filter.h"
#endif

using namespace ankerl;
using namespace std::chrono_literals;

template <typename T>
void RunFilter(std::span<const float> input, std::span<float> output, size_t block_size)
{
    assert(input.size() % block_size == 0);
    T filter;

    size_t block_count = input.size() / block_size;
    for (size_t i = 0; i < block_count; ++i)
    {
        auto input_block = input.subspan(i * block_size, block_size);
        auto output_block = output.subspan(i * block_size, block_size);
        filter.process(input_block, output_block);
    }
}

template <typename T>
void RunTest(const std::string& name)
{
    const std::vector<size_t> kBlockSize = {1, 4, 8, 16, 32, 64, 128, 256, 512, 1024};
    constexpr size_t input_size = 32768;

    nanobench::Bench bench;
    bench.title(name);
    bench.relative(true);
    bench.warmup(100);
    bench.batch(input_size);
    bench.unit("samples");

    std::vector<float> input(input_size, 0);

    // Fill with white noise
    std::default_random_engine generator;
    std::normal_distribution<double> dist(0, 0.1);
    for (size_t i = 0; i < input.size(); ++i)
    {
        input[i] = dist(generator);
    }

    std::vector<float> output(input_size, 0);

    for (size_t i = 0; i < kBlockSize.size(); ++i)
    {
        // bench.batch(kBlockSize[i]);
        std::string test_name = name + "_" + std::to_string(kBlockSize[i]);
        bench.run(test_name, [&]() { RunFilter<T>(input, output, kBlockSize[i]); });
    }

    std::string filename = "perf_results_" + name + ".json";
    std::ofstream render_out(filename);
    bench.render(ankerl::nanobench::templates::json(), render_out);
}

int main()
{
    std::cout << "Running filter benchmarks..." << std::endl;

#ifdef BASIC_FILTER
    RunTest<BasicFilter>("basic_filter");
    RunTest<CascadedIIRDF2T>("CascadedIIRDF2T");
    RunTest<CascadedIIRDF1>("CascadedIIRDF1");
#endif

#ifdef CMSIS_FILTER_SCALAR
    RunTest<CMSISFilterDF2T>("CMSIS_Scalar_FilterDF2T");
    RunTest<CMSISFilterDF1>("CMSIS-Scalar_FilterDF1");
#endif

#ifdef VDSP_FILTER
    RunTest<vDSPFilter>("vdsp_filter");
#endif

#ifdef CMSIS_FILTER_NEON
    RunTest<CMSISFilterDF2T>("CMSIS_NEON_FilterDF2T");
    RunTest<CMSISFilterDF1>("CMSIS_NEON_FilterDF1");
#endif
}