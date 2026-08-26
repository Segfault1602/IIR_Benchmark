#include "mc_simd_bank.h"

#include <memory>
#include <stdexcept>

void McSimdBank::prepare(std::span<const std::array<float, 6>> sos, uint32_t channels,
                         uint32_t stages)
{
    if (channels == 0 || stages == 0)
    {
        channels_ = channels;
        stages_ = stages;
        coeffs_.clear();
        bank_.Clear();
        return;
    }

    const size_t section_count = static_cast<size_t>(channels) * static_cast<size_t>(stages);
    if (sos.size() != section_count)
    {
        throw std::invalid_argument("sos size does not match channels * stages");
    }

    channels_ = channels;
    stages_ = stages;
    coeffs_.clear();
    coeffs_.reserve(section_count);

    for (const auto& section : sos)
    {
        coeffs_.push_back({section[0], section[1], section[2], section[3], section[4], section[5]});
    }

    bank_.SetCoefficients(coeffs_, channels);
}

void McSimdBank::process(std::span<const float> input, std::span<float> output, uint32_t channels,
                        uint32_t frames)
{
    if (channels != channels_ || frames == 0)
    {
        return;
    }

    if (input.size() < static_cast<size_t>(channels) * frames || output.size() < static_cast<size_t>(channels) * frames)
    {
        throw std::invalid_argument("input/output buffer too small");
    }

    // The bank only reads its input span and never writes back to it when the source/destination
    // buffers differ, so the const-cast is safe here.
    std::span<float> mutable_input = std::span<float>(const_cast<float*>(input.data()), input.size());
    sfFDN::AudioBuffer ib(frames, channels, mutable_input);
    sfFDN::AudioBuffer ob(frames, channels, output);
    bank_.Process(ib, ob);
}

const char* McSimdBank::name() const
{
    // simd.h picks the kernel at compile time, so report whichever one this translation unit was
    // actually built with. The x86 build compiles this file once per width, which is why the width
    // has to appear in the benchmark name.
#if defined(SFFDN_SIMD_AVX)
    return "SimdBiquadBank_AVX";
#elif defined(SFFDN_SIMD_SSE)
    return "SimdBiquadBank_SSE";
#elif defined(SFFDN_SIMD_NEON)
    return "SimdBiquadBank_NEON";
#else
    return "SimdBiquadBank_Scalar";
#endif
}

#ifdef MC_SIMD_BANK_FACTORY
// Each variant is compiled with the namespace and class renamed, so the only symbol the rest of the
// program can name is this factory. See sffdn_add_bank_variant() in CMakeLists.txt.
std::unique_ptr<MultiChannelFilter> MC_SIMD_BANK_FACTORY()
{
    return std::make_unique<McSimdBank>();
}
#endif
