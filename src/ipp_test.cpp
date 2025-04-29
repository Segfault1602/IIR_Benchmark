#include <cassert>
#include <iostream>
#include <vector>

#include <ipp.h>
// #include <ipps.h>

#include "filter_coeffs.h"

int main()
{
    const IppLibraryVersion* libVersion;
    IppStatus status;
    Ipp64u cpuFeatures, enabledFeatures;

    // ippInit(); /* Initialize Intel® IPP library */

    IppsIIRState_32f* pIIRState = nullptr;
    Ipp8u* pBuffer = nullptr;
    Ipp32f* pTaps = nullptr;
    Ipp32f* pDlyLine = nullptr;
    int numBq = kTestSOS.size();

    int bufferSize = 0;
    status = ippsIIRGetStateSize_BiQuad_32f(numBq, &bufferSize);
    if (status != ippStsNoErr)
    {
        std::cerr << "Error: " << status << std::endl;
        return -1;
    }

    pBuffer = ippsMalloc_8u(bufferSize);
    if (pBuffer == nullptr)
    {
        std::cerr << "Error: Unable to allocate memory for buffer" << std::endl;
        return -1;
    }

    std::vector<Ipp32f> taps;
    for (size_t i = 0; i < kTestSOS.size(); ++i)
    {
        for (size_t j = 0; j < kTestSOS[i].size(); ++j)
        {
            taps.push_back(kTestSOS[i][j]);
        }
    }
    assert(taps.size() == 6 * numBq);
    pTaps = taps.data();

    std::vector<Ipp32f> dlyLine(2 * numBq, 0.0f);
    pDlyLine = dlyLine.data();

    status = ippsIIRInit_BiQuad_32f(&pIIRState, pTaps, numBq, pDlyLine, pBuffer);
    if (status != ippStsNoErr)
    {
        std::cerr << "Error: " << status << std::endl;
        ippsFree(pBuffer);
        return -1;
    }

    constexpr size_t size = 32;
    std::array<float, size> input = {0};
    input[0] = 1.f;
    std::array<float, size> output;

    status = ippsIIR_32f(input.data(), output.data(), size, pIIRState);
    if (status != ippStsNoErr)
    {
        std::cerr << "Error: " << status << std::endl;
        ippsFree(pBuffer);
        return -1;
    }

    for (size_t i = 0; i < size; ++i)
    {
        std::cout << output[i] << " ";
    }
    std::cout << std::endl;

    ippsFree(pBuffer);
    return 0;
}