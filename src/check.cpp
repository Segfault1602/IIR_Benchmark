#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>

#include "basic_filter.h"
#include "cmsis_filter.h"
#include "filter_coeffs.h"
#include <cassert>
#include <sndfile.h>
#include <vector>

#ifdef __APPLE__
#include "vdsp_filter.h"
#endif

#ifdef IPP_FILTER
#include "ipp_filter.h"
#endif

#ifdef STEAMAUDIO_FILTER
#include "steamaudio_filter.h"
#endif

TEST_CASE_TEMPLATE_DEFINE("Filter", T, test_id)
{
    SF_INFO in_info;
    SNDFILE* in_file = sf_open("x1.wav", SFM_READ, &in_info);
    if (!in_file)
    {
        std::cerr << "Error opening file: " << sf_strerror(in_file) << std::endl;
        return;
    }
    assert(in_info.channels == 1);

    std::vector<float> input_buffer(in_info.frames);
    size_t read_frames = sf_readf_float(in_file, input_buffer.data(), input_buffer.size());
    if (read_frames != input_buffer.size())
    {
        std::cerr << "Error reading file: " << sf_strerror(in_file) << std::endl;
        sf_close(in_file);
        return;
    }
    sf_close(in_file);

    std::vector<float> output_buffer(input_buffer.size(), 0.f);

    T filter;
    constexpr size_t kBlockSize = 128;
    size_t i = 0;
    while ((i + kBlockSize) < input_buffer.size())
    {
        filter.process({input_buffer.data() + i, kBlockSize}, {output_buffer.data() + i, kBlockSize});
        i += kBlockSize;
    }

    if (i < input_buffer.size())
    {
        filter.process({input_buffer.data() + i, input_buffer.size() - i},
                       {output_buffer.data() + i, input_buffer.size() - i});
    }

    SF_INFO out_info;
    SNDFILE* expected_file = sf_open("x1_filtered.wav", SFM_READ, &out_info);
    if (!expected_file)
    {
        std::cerr << "Error opening file: " << sf_strerror(expected_file) << std::endl;
        return;
    }
    assert(out_info.channels == 1);

    std::vector<float> expected_buffer(input_buffer.size(), 0.f);
    size_t read_expected_frames = sf_readf_float(expected_file, expected_buffer.data(), expected_buffer.size());
    if (read_expected_frames != expected_buffer.size())
    {
        std::cerr << "Error reading file: " << sf_strerror(expected_file) << std::endl;
        sf_close(expected_file);
        return;
    }

    sf_close(expected_file);

    float signal_energy = 0.f;
    float signal_error = 0.f;

    for (size_t j = 0; j < expected_buffer.size(); ++j)
    {
        CHECK_EQ(output_buffer[j], doctest::Approx(expected_buffer[j]).epsilon(1e-5));
        signal_energy += expected_buffer[j] * expected_buffer[j];
        signal_error += (output_buffer[j] - expected_buffer[j]) * (output_buffer[j] - expected_buffer[j]);
    }

    float snr = 10.f * log10(signal_energy / signal_error);
    std::cout << "SNR for " << typeid(filter).name() << ": " << snr << " dB" << std::endl;
}

TEST_CASE_TEMPLATE_INVOKE(test_id, CMSISFilterDF2T, CMSISFilterDF1, BasicFilter, CascadedIIRDF2T, CascadedIIRDF1
#ifdef __APPLE__
                          ,
                          vDSPFilter
#endif
#ifdef IPP_FILTER
                          ,
                          IppFilter
#endif
#ifdef STEAMAUDIO_FILTER
                          ,
                          SteamAudioFilter
#endif
);
