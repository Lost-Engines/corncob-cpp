#include "recorded_flight.h"
#include <bit>
#include <stdexcept>
#include "maths/integer.h"
#include "resources/binary_reader.h"

namespace corncob::game {

recorded_flight::recorded_flight(std::span<uint8_t const> const bytes) {
  /// COMPACTREAD stores either a 37-byte full state or signed acceleration bytes and ten-bit angles.
  resources::binary_reader const input{bytes};
  recorded_frame current;
  bool have_state{false};
  unsigned int offset{0};
  while(offset < bytes.size()) {
    uint8_t const tag{input.byte(offset)};
    if(tag == 255) {
      if(bytes.size() - offset != 37 || motion.empty()) throw std::invalid_argument{"invalid recorded-flight terminator"};
      return;
    }
    if((tag & 8u) == 0) throw std::invalid_argument{"invalid recorded-flight control byte"};
    bool const compact{(tag & 128u) != 0};
    unsigned int const length{compact ? 8u : 37u};
    if(length > bytes.size() - offset) throw std::invalid_argument{"truncated recorded flight"};
    if(compact) {
      if(!have_state) throw std::invalid_argument{"recorded-flight delta precedes its full state"};
      for(unsigned int axis{0}; axis != 3; ++axis) {
        int8_t const acceleration{std::bit_cast<int8_t>(input.byte(offset + 1 + axis))};
        int16_t const velocity{maths::word(static_cast<int32_t>(maths::word(current.velocity[axis])) + acceleration)};
        current.velocity[axis] = maths::dword((static_cast<uint32_t>(current.velocity[axis]) & 0xffff0000u) | static_cast<uint16_t>(velocity));
        current.position[axis] = maths::dword(static_cast<int64_t>(current.position[axis]) + velocity);
      }
      uint8_t const low{input.byte(offset + 7)};
      current.attitude = {
        .heading{static_cast<uint16_t>((input.byte(offset + 5) << 8) | ((low >> 2) & 3u) << 6)},
        .pitch{static_cast<uint16_t>((input.byte(offset + 4) << 8) | ((low >> 4) & 3u) << 6)},
        .roll{static_cast<uint16_t>((input.byte(offset + 6) << 8) | (low & 3u) << 6)},
      };
    } else if((tag & 32u) == 0) {
      current.position = maths::world_position{input.signed_dword(offset + 1), input.signed_dword(offset + 5), input.signed_dword(offset + 9)};
      current.attitude = {
        .heading{input.word(offset + 15)},
        .pitch{input.word(offset + 13)},
        .roll{input.word(offset + 17)},
      };
      current.velocity = vec3<int32_t>{input.signed_dword(offset + 19), input.signed_dword(offset + 23), input.signed_dword(offset + 27)};
      have_state = true;
    } else if(!have_state) throw std::invalid_argument{"recorded flight begins with an ejection-only record"};
    current.buttons = tag;
    motion.push_back(current);
    offset += length;
  }
  throw std::invalid_argument{"recorded flight has no terminator"};
}
std::span<recorded_frame const> recorded_flight::frames() const noexcept {
  return motion;
}

} // namespace corncob::game
