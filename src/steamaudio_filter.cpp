#include "steamaudio_filter.h"

#include "filter_coeffs.h"

SteamAudioFilter::SteamAudioFilter(size_t num_stage)
{
    size_t stage = num_stage == 0 ? kTestSOS.size() : num_stage;
    filters_.resize(stage);

    for (size_t i = 0; i < stage; i++)
    {
        ipl::IIR iir;
        iir.b0 = kTestSOS[i % kTestSOS.size()][0] / kTestSOS[i % kTestSOS.size()][3];
        iir.b1 = kTestSOS[i % kTestSOS.size()][1] / kTestSOS[i % kTestSOS.size()][3];
        iir.b2 = kTestSOS[i % kTestSOS.size()][2] / kTestSOS[i % kTestSOS.size()][3];
        iir.a1 = kTestSOS[i % kTestSOS.size()][4] / kTestSOS[i % kTestSOS.size()][3];
        iir.a2 = kTestSOS[i % kTestSOS.size()][5] / kTestSOS[i % kTestSOS.size()][3];

        filters_[i].setFilter(iir);
    }
}

void SteamAudioFilter::process(std::span<const float> input, std::span<float> output)
{
    for (size_t i = 0; i < filters_.size(); i++)
    {
        filters_[i].apply(input.size(), input.data(), output.data());
        input = output;
    }
}