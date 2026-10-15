#include "world.h"
#include <stdexcept>
#include <utility>
#include "resources/binary_reader.h"

namespace corncob::world {

theatre decode_theatre(std::span<uint8_t const> const source) {
  /// Expand the original 64-tile CCT stream while retaining its tile and palette boundaries
  unsigned int constexpr header_bytes{16};
  unsigned int constexpr record_bytes{23};
  resources::binary_reader const input{source};
  theatre result;
  size_t offset{0};
  for(unsigned int y{0}; y != theatre::rows; ++y) {
    for(unsigned int x{0}; x != theatre::columns; ++x) {
      if(input.dword(offset) != 0x00'65'69'50u || input.word(offset + 4) != x || input.word(offset + 6) != y) {
        throw std::invalid_argument{"embedded theatre has an invalid tile header"};
      }
      auto &tile{result.tiles[y * theatre::columns + x]};
      tile.first_object = static_cast<unsigned int>(result.objects.size());
      tile.object_count = input.word(offset + 8);
      tile.storage_flags = input.byte(offset + 10);
      for(unsigned int i{0}; i != tile.packed_colours.size(); ++i) tile.packed_colours[i] = input.byte(offset + 11 + i);
      offset += header_bytes;
      for(unsigned int i{0}; i != tile.object_count; ++i) {
        auto const packed{input.word(offset)};
        auto const low_x{input.word(offset + 2)}, low_y{input.word(offset + 4)}, low_z{input.word(offset + 6)};
        auto const kind{static_cast<uint16_t>(input.word(offset + 21) ^ low_x ^ low_y)};
        if(kind >= std::to_underlying(object_type::count)) throw std::invalid_argument{"embedded theatre contains an invalid object type"};
        result.objects.push_back({
          .position{
            theatre::origin + static_cast<int32_t>(x) * theatre::tile_size + static_cast<int32_t>(packed & 31) * 65'536 + low_x,
            theatre::origin + static_cast<int32_t>(y) * theatre::tile_size + static_cast<int32_t>((packed >> 5) & 31) * 65'536 + low_y,
            static_cast<int32_t>((packed >> 10) & 31) * 65'536 + low_z,
          },
          .attitude{
            .heading{static_cast<maths::angle>(input.byte(offset + 8) << 8)},
            .pitch{static_cast<maths::angle>(input.byte(offset + 9) << 8)},
            .roll{static_cast<maths::angle>(input.byte(offset + 10) << 8)},
          },
          .velocity{input.signed_word(offset + 13), input.signed_word(offset + 15), input.signed_word(offset + 17)},
          .type{static_cast<object_type>(kind)},
          .flags{input.word(offset + 11)},
          .timer{input.word(offset + 19)},
        });
        offset += record_bytes;
      }
    }
  }
  if(offset > source.size() || source.size() - offset != 12) throw std::invalid_argument{"embedded theatre has an invalid start-position trailer"};
  result.start = maths::world_position{input.signed_dword(offset), input.signed_dword(offset + 4), input.signed_dword(offset + 8)};
  return result;
}

} // namespace corncob::world
