#pragma once

#include <cstdint>

namespace corncob::maths {

class random_source {
private:
  uint32_t state;

public:
  explicit random_source(uint32_t const seed = 2'949'243u) noexcept : state{seed} {
    /// Start the original wrapping multiplicative random stream
  }
  uint16_t next() noexcept {
    /// RAND_W exposes the high word of the updated dword state
    state *= 0x278dde6du;
    return static_cast<uint16_t>(state >> 16);
  }
  uint16_t below(uint16_t const limit) noexcept {
    /// RANDR_W keeps the remainder, retaining the unrestricted result for a zero range
    auto const value{next()};
    return limit == 0 ? value : static_cast<uint16_t>(value % limit);
  }
  uint32_t seed() const noexcept {
    /// Expose the continuation state for saves and native comparisons
    return state;
  }
};

} // namespace corncob::maths
