#include "ipp_filter.h"

#include <cassert>
#include <cstring>
#include <iostream>
#include <vector>

#include "filter_coeffs.h"

IppFilter::IppFilter(size_t num_stage)
{
    int numBq = num_stage == 0 ? static_cast<int>(kTestSOS.size()) : static_cast<int>(num_stage);

    int bufferSize = 0;
    IppStatus status = ippsIIRGetStateSize_BiQuad_32f(numBq, &bufferSize);
    if (status != ippStsNoErr)
    {
        std::cerr << "Error: " << status << std::endl;
        return;
    }

    pBuffer_ = ippsMalloc_8u(bufferSize);
    if (pBuffer_ == nullptr)
    {
        std::cerr << "Error: Unable to allocate memory for buffer" << std::endl;
        return;
    }

    pTaps_ = ippsMalloc_32f(6 * numBq);
    for (int i = 0; i < numBq; ++i)
    {
        const auto& sos = kTestSOS[i % kTestSOS.size()];
        for (size_t j = 0; j < sos.size(); ++j)
        {
            pTaps_[i * 6 + j] = sos[j];
        }
    }

    pDlyLine_ = ippsMalloc_32f(2 * numBq);
    memset(pDlyLine_, 0, sizeof(Ipp32f) * 2 * numBq);

    status = ippsIIRInit_BiQuad_32f(&pIIRState_, pTaps_, numBq, pDlyLine_, pBuffer_);
    if (status != ippStsNoErr)
    {
        std::cerr << "Error: " << status << std::endl;
        ippsFree(pBuffer_);
        return;
    }
}

IppFilter::~IppFilter()
{
    // if (pIIRState_ != nullptr)
    // {
    //     ippsFree(pIIRState_);
    //     pIIRState_ = nullptr;
    // }

    if (pBuffer_ != nullptr)
    {
        ippsFree(pBuffer_);
        pBuffer_ = nullptr;
    }

    if (pTaps_ != nullptr)
    {
        ippsFree(pTaps_);
        pTaps_ = nullptr;
    }

    if (pDlyLine_ != nullptr)
    {
        ippsFree(pDlyLine_);
        pDlyLine_ = nullptr;
    }
}

void IppFilter::process(std::span<const float> input, std::span<float> output)
{
    assert(input.size() == output.size());

    IppStatus status = ippsIIR_32f(input.data(), output.data(), static_cast<int>(input.size()), pIIRState_);
    if (status != ippStsNoErr)
    {
        std::cerr << "Error: " << status << std::endl;
    }
}