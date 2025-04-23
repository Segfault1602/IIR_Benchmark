#pragma once

#include <span>

class Filter
{
  public:
    Filter() = default;
    virtual ~Filter() = default;

    virtual void process(std::span<const float> input, std::span<float> output) = 0;
};