#pragma once

#include "filter.h"

#include <ipp.h>

class IppFilter : public Filter
{
  public:
    IppFilter();
    ~IppFilter() override;

    void process(std::span<const float> input, std::span<float> output) override;

  private:
    IppsIIRState_32f* pIIRState_ = nullptr;
    Ipp8u* pBuffer_ = nullptr;
    Ipp32f* pTaps_ = nullptr;
    Ipp32f* pDlyLine_ = nullptr;
};