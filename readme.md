# Filter Benchmark

This repository contains benchmarks for different IIR filter implementations in C++.
The different implementations are:
- **CMSIS Scalar DF2T**: A scalar implementation of the Direct Form II Transposed (DF2T) structure using CMSIS DSP library.
- **CMSIS Scalar DF1** : A scalar implementation of the Direct Form I (DF1) structure using CMSIS DSP library.
- **Basic Filter** : A naive implementation of the Direct Form I (DF1) structure using C++.
- **Cascaded IIR DF2T** : A scalar implementation of the DF2T structure, but as opposed to the CMSIS implementation, each samples is processed by every stages before moving the next sample.
- **Cascaded IIR DF1** : A scalar implementation of the DF1 structure, but as opposed to the CMSIS implementation, each samples is processed by every stages before moving the next sample.
- **vDSP** : A vectorized implementation of the Direct Form I (DF1) structure using Apple's Accelerate framework.
- **IPP** : A vectorized implementation of the Direct Form II (DF2) structure using Intel's Integrated Performance Primitives (IPP) library (`ippsIIRInit_BiQuad_32f`).

## Methodology

The benchmarking test consists of filtering 32768 samples of white noise with a specific block size. The test filter is composed of 12 cascaded biquads.
```cpp
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
```

The measurements are done using the [nanobench](https://nanobench.ankerl.com/) library.

## Results

### ARM (MacOS)

The following results where obtained by running the benchmark on a 2024 Macbook Air with M3 chip. The plots show the time taken to process 1 sample. The batch size indicates the number of samples processed by the filter at once. Compiled with **AppleClang 17.0.0.17000013** with the `-O3` optimization flag.


![Benchmark Results](results/perf_results_darwin.png)

### Intel (Windows)

The following results where obtained by running the benchmark on an Intel i9-12900K CPU with the following specs:

|         |        |
|:-----------|:-----------|
| Core Speed | 4900 MHz |
| L1 Data | 8 x 48 KB + 8x32 KB|
| L1 Inst. | 8 x 23 KB + 8 x 64 KB|
| L2 Cache | 8 x 1.25 MB + 2 x 2 MB |
| L3 Cache | 30 MB |

Compiled with **Clang 20.1.3** with the `-O3` optimization flag.

> [!NOTE]
> The IPP implementation is struggling with a block size less than 64 but if a dummy biquad is added to the filter, to bring its order to 12, the performance becomes linear at around 10 ns. It seems that the number of cascaded biquads needs to be a multiple of 2 for the IPP performance to be optimal. Needs to be investigated further.

![Benchmark Results](results/perf_results_win32.png)
