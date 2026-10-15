#pragma once

#include <array>
#include <bit>
#include <cstdint>
#include "vectorstorm/vector/vector3.h"

namespace corncob::maths {

using angle = uint16_t;
using coordinate = int32_t;
using coefficient = int16_t;
using world_position = vec3<coordinate>;
using fixed_vector = vec3<coefficient>;

struct angles {
  angle heading{0};
  angle pitch{0};
  angle roll{0};
};

struct rotation {
  std::array<fixed_vector, 3> rows;
};

constexpr coefficient word(int32_t const value) noexcept {
  /// Preserve the low word of native arithmetic without implementation-defined signed conversion
  return std::bit_cast<int16_t>(static_cast<uint16_t>(value));
}

constexpr coefficient negate(coefficient const value) noexcept {
  /// Match the original word NEG, including its most-negative input
  return word(-static_cast<int32_t>(value));
}

constexpr coefficient multiply(coefficient const lhs, coefficient const rhs) noexcept {
  /// The original HMUL keeps product bits 15..30 rather than rounding a floating-point product
  return word((static_cast<int32_t>(lhs) * rhs) >> 15);
}

coefficient add_saturated(coefficient lhs, coefficient rhs) noexcept;
coefficient sine(angle value) noexcept;
coefficient cosine(angle value) noexcept;
uint16_t sine_magnitude(angle value) noexcept;
angle arctangent(coefficient x, coefficient y) noexcept;
uint16_t magnitude(fixed_vector const &value) noexcept;
rotation make_rotation(angles const &value) noexcept;
rotation make_inverse_rotation(angles const &value) noexcept;
fixed_vector transform(rotation const &matrix, fixed_vector const &value) noexcept;
fixed_vector transform_vertex(rotation const &matrix, fixed_vector const &value) noexcept;
rotation compose(rotation const &lhs, rotation const &rhs) noexcept;
angles extract_angles(rotation const &body_to_world) noexcept;
rotation rotate_local(rotation const &body_to_world, angles const &change) noexcept;

} // namespace corncob::maths
