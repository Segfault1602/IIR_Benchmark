#pragma once

#include "sffdn_shim.h"

#include <cstdint>
#include <span>

namespace sfFDN
{
class AudioBuffer
{
  public:
    AudioBuffer() noexcept SFFDN_NONBLOCKING;

    AudioBuffer(uint32_t frame_size, uint32_t channels, std::span<float> buffer) noexcept SFFDN_NONBLOCKING;

    uint32_t SampleCount() const noexcept SFFDN_NONBLOCKING;

    uint32_t ChannelCount() const noexcept SFFDN_NONBLOCKING;

    std::span<const float> GetChannelSpan(uint32_t channel) const noexcept SFFDN_NONBLOCKING;

    std::span<float> GetChannelSpan(uint32_t channel) noexcept SFFDN_NONBLOCKING;

    AudioBuffer Offset(uint32_t offset, uint32_t frame_size) const noexcept SFFDN_NONBLOCKING;

  private:
    uint32_t frame_size_;
    uint32_t channel_count_;
    std::span<float> buffer_;
    uint32_t offset_;
    uint32_t chunk_size_;
};
} // namespace sfFDN
