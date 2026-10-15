#pragma once

#include <bit>
#include <cstdint>
#include <span>
#include <stdexcept>
#include "maths/fixed.h"

namespace corncob::resources {

class binary_reader {
private:
  std::span<uint8_t const> source;

public:
  binary_reader(std::span<uint8_t const> source) :
    source{source} {
    /// Keep resource decoding independent of native alignment, endian order and structure padding
  }

  uint8_t byte(size_t const offset) const {
    /// Read one validated byte from an original resource
    if(offset >= source.size()) throw std::invalid_argument{"original resource byte exceeds its embedded data"};
    return source[offset];
  }
  uint16_t word(size_t const offset) const {
    /// Decode an original little-endian word
    return static_cast<uint16_t>(static_cast<unsigned int>(byte(offset)) | (static_cast<unsigned int>(byte(offset + 1)) << 8));
  }
  int16_t signed_word(size_t const offset) const {
    /// Preserve the original signed interpretation of a word's bits
    return std::bit_cast<int16_t>(word(offset));
  }
  uint32_t dword(size_t const offset) const {
    /// Decode an original little-endian double word
    return static_cast<uint32_t>(word(offset)) | (static_cast<uint32_t>(word(offset + 2)) << 16);
  }
  int32_t signed_dword(size_t const offset) const {
    /// Preserve a signed coordinate without a host-dependent narrowing conversion
    return std::bit_cast<int32_t>(dword(offset));
  }
  maths::fixed_vector vector(size_t const offset) const {
    /// Decode a stored three-word model vector
    return {signed_word(offset), signed_word(offset + 2), signed_word(offset + 4)};
  }
};

} // namespace corncob::resources
