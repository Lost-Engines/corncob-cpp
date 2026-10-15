#include "fixed.h"
#include <algorithm>
#include "maths/arctangent_table.h"
#include "maths/sine_table.h"

namespace corncob::maths {

coefficient add_saturated(coefficient const lhs, coefficient const rhs) noexcept {
  /// MADD halves before adding, clamps, then rotates: negative sums retain a low sign bit
  int constexpr half_limit{16'383};
  int sum{(static_cast<int>(lhs) >> 1) + (static_cast<int>(rhs) >> 1)};
  // JS/JNS inspect the wrapped subtraction sign without correcting for overflow
  if(word(sum - half_limit) >= 0) sum = half_limit;
  else if(word(sum + half_limit) < 0) sum = -half_limit;
  return std::bit_cast<int16_t>(std::rotl(static_cast<uint16_t>(sum), 1));
}

uint16_t sine_magnitude(angle const value) noexcept {
  /// Raw SIN returns a full unsigned magnitude; the atmosphere code uses this extra bit of precision
  unsigned int constexpr half_turn{32'768};
  unsigned int constexpr quarter_turn{16'384};
  unsigned int constexpr table_fraction_bits{6};
  unsigned int constexpr table_step{1u << table_fraction_bits};
  if((value & (half_turn - 1)) == 0) return 0;
  bool const negative{(value & half_turn) != 0};
  unsigned int phase{negative ? static_cast<unsigned int>(static_cast<uint16_t>(-value)) : value};
  if((phase & quarter_turn) != 0) phase = half_turn - phase;
  unsigned int magnitude{65'535};
  if(phase != quarter_turn) {
    auto const index{phase >> table_fraction_bits};
    magnitude = sine_quarter[index] + ((sine_quarter[index + 1] - sine_quarter[index]) * (phase & (table_step - 1))) / table_step;
  }
  return static_cast<uint16_t>(magnitude);
}
coefficient sine(angle const value) noexcept {
  /// Signed SSIN halves the raw magnitude before restoring the angle sign
  auto const result{static_cast<coefficient>(sine_magnitude(value) >> 1)};
  return (value & 0x8000u) != 0 ? negate(result) : result;
}
coefficient cosine(angle const value) noexcept {
  /// The original cosine entry adds a quarter turn and jumps into signed sine
  return sine(static_cast<angle>(value + 16'384u));
}

angle arctangent(coefficient x, coefficient y) noexcept {
  /// ATN2 selects a quantised octant table; its zero vector deliberately returns an eighth turn
  if(x == -32'768) ++x;
  uint8_t quadrant{0}, quarter{0};
  if(x < 0) {
    quadrant = 3;
    x = negate(x);
  }
  if(y < 0) {
    quadrant ^= 5;
    y = negate(y);
  }
  unsigned int const abs_x{static_cast<uint16_t>(x)}, abs_y{static_cast<uint16_t>(y)};
  uint16_t value{32'768};
  if(abs_x != abs_y) {
    unsigned int const larger{std::max(abs_x, abs_y)}, smaller{std::min(abs_x, abs_y)};
    if(abs_x < abs_y) {
      quarter = 64;
      quadrant ^= 1;
      quadrant = static_cast<uint8_t>(((quadrant >> 2) << 1) | (quadrant & 1));
    }
    unsigned int const ratio{((smaller << 16) / larger) >> 1};
    value = arctangent_table[ratio >> 5];
  }
  int result{value >> 2};
  if((quadrant & 1u) != 0) result = -result;
  if((quadrant & 2u) != 0) quarter = static_cast<uint8_t>(quarter + 128);
  return static_cast<angle>(result + (quarter << 8));
}

rotation make_rotation(angles const &value) noexcept {
  /// Translate F3DVEC's CALCMAT with its original sequence of Q15 products and saturated additions
  coefficient const sy{sine(value.heading)}, cy{cosine(value.heading)};
  coefficient const sp{sine(value.pitch)}, cp{cosine(value.pitch)};
  coefficient const sr{sine(value.roll)}, cr{cosine(value.roll)};
  return {
    .rows{
      fixed_vector{multiply(cy, cp), multiply(cp, sy), negate(sp)},
      fixed_vector{add_saturated(multiply(multiply(sr, sp), cy), negate(multiply(cr, sy))),
                   add_saturated(multiply(multiply(sr, sp), sy), multiply(cr, cy)), multiply(cp, sr)},
      fixed_vector{add_saturated(multiply(multiply(cr, sp), cy), multiply(sr, sy)),
                   add_saturated(multiply(multiply(cr, sp), sy), negate(multiply(sr, cy))), multiply(cp, cr)},
    },
  };
}
rotation make_inverse_rotation(angles const &value) noexcept {
  /// NCALCMAT negates the sine terms; transposing an already rounded matrix is not equivalent
  coefficient const sy{negate(sine(value.heading))}, cy{cosine(value.heading)};
  coefficient const sp{negate(sine(value.pitch))}, cp{cosine(value.pitch)};
  coefficient const sr{negate(sine(value.roll))}, cr{cosine(value.roll)};
  coefficient const cysp{multiply(cy, sp)}, sysp{multiply(sy, sp)};
  return {
    .rows{
      fixed_vector{multiply(cy, cp), add_saturated(multiply(cysp, sr), multiply(sy, cr)),
                   add_saturated(multiply(sy, sr), negate(multiply(cysp, cr)))},
      fixed_vector{negate(multiply(sy, cp)), add_saturated(multiply(cy, cr), negate(multiply(sysp, sr))),
                   add_saturated(multiply(sysp, cr), multiply(cy, sr))},
      fixed_vector{sp, negate(multiply(cp, sr)), multiply(cp, cr)},
    },
  };
}

fixed_vector transform(rotation const &matrix, fixed_vector const &value) noexcept {
  /// MATVMUL saturates after each pair of products, matching aircraft forces and object origins
  fixed_vector result;
  for(unsigned int row{0}; row != 3; ++row) {
    auto const &axis{matrix.rows[row]};
    result[row] = add_saturated(add_saturated(multiply(value.x, axis.x), multiply(value.y, axis.y)), multiply(value.z, axis.z));
  }
  return result;
}
fixed_vector transform_vertex(rotation const &matrix, fixed_vector const &value) noexcept {
  /// The fast drawing path rotates each product's high word; it deliberately differs from HMUL
  auto product{[](coefficient const lhs, coefficient const rhs){
    auto const high{static_cast<uint16_t>((static_cast<int32_t>(lhs) * rhs) >> 16)};
    return std::bit_cast<int16_t>(std::rotl(high, 1));
  }};
  fixed_vector result;
  for(unsigned int row{0}; row != 3; ++row) {
    auto const &axis{matrix.rows[row]};
    result[row] = word(static_cast<int32_t>(product(value.x, axis.x)) + product(value.y, axis.y) + product(value.z, axis.z));
  }
  return result;
}
rotation compose(rotation const &lhs, rotation const &rhs) noexcept {
  /// DOT2 accumulates signed products as a wrapping dword before its high-word clamp
  rotation result;
  for(unsigned int row{0}; row != 3; ++row) {
    for(unsigned int column{0}; column != 3; ++column) {
      uint32_t total{0};
      for(unsigned int inner{0}; inner != 3; ++inner) total += static_cast<uint32_t>(static_cast<int32_t>(lhs.rows[row][inner]) * rhs.rows[inner][column]);
      int const high{std::bit_cast<int16_t>(static_cast<uint16_t>(total >> 16))};
      int const bounded{high >= 16'383 ? 16'382 : high <= -16'383 ? -16'382 : high};
      total = (static_cast<uint32_t>(static_cast<uint16_t>(bounded)) << 16) | (total & 0xffffu);
      result.rows[row][column] = word(std::bit_cast<int32_t>(total << 1) >> 16);
    }
  }
  return result;
}

} // namespace corncob::maths
