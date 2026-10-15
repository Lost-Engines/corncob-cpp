#pragma once

#include <bit>
#include <cassert>
#include <cstdint>
#include <cstdlib>

namespace corncob::maths {

constexpr int32_t dword(int64_t const value) noexcept {
  /// Preserve original dword wrap without C++ signed overflow
  return std::bit_cast<int32_t>(static_cast<uint32_t>(value));
}
constexpr int32_t scale(int32_t const value, int16_t const factor) noexcept {
  /// MULS_DW retains the low dword of a signed dword-by-word product
  return dword(static_cast<int64_t>(value) * factor);
}
inline int32_t rounded_divide(int32_t const numerator, int16_t const denominator) {
  /// DIVRS_DW rounds absolute quotients to nearest, with ties away from zero
  assert(denominator != 0);
  int64_t const dividend{std::abs(static_cast<int64_t>(numerator))};
  int const divisor{std::abs(static_cast<int>(denominator))};
  int64_t const quotient{(dividend + divisor / 2) / divisor};
  return dword((numerator < 0) != (denominator < 0) ? -quotient : quotient);
}

} // namespace corncob::maths
