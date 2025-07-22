#include "neon_filter.h"
#include "steamaudio_filter.h"

#include <array>
#include <iostream>

int main()
{
    NeonIIRFilter neon_filter(3);
    SteamAudioFilter steam_filter(3);

    constexpr size_t KSize = 5;

    std::array<float, KSize> input = {0.f};
    input[0] = 1.f;
    std::array<float, KSize> output_neon = {0.f};
    std::array<float, KSize> output_steam = {0.f};

    neon_filter.process(input, output_neon);

    steam_filter.process(input, output_steam);
    for (size_t i = 0; i < output_steam.size(); ++i)
    {
        std::cout << "Steam[" << i << "]: " << output_steam[i] << ", Neon[" << i << "]: " << output_neon[i]
                  << std::endl;
    }

    return 0;
}