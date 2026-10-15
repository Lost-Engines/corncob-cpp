#include "world.h"
#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <utility>
#include "resources/binary_reader.h"

namespace corncob::world {

bool in_active_neighbourhood(maths::world_position const &position, maths::world_position const &observer) noexcept {
  /// UPDATECT streams the four tiles surrounding a half-tile-rounded observer centre
  for(unsigned int axis{0}; axis != 2; ++axis) {
    int const centre{((observer[axis] >> 16) - (theatre::origin >> 16) + 16) / 32};
    int const tile{((position[axis] >> 16) - (theatre::origin >> 16)) / 32};
    if(tile < 0 || tile >= 8 || centre - tile < 0 || centre - tile > 1) return false;
  }
  return true;
}

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

void apply_briefing(theatre &theatre, std::span<uint8_t const> const source) {
  /// TWR's optional first-line X directive controls the actual sun, atmosphere and star settings
  theatre.briefing.assign(reinterpret_cast<char const*>(source.data()), source.size());
  if(source.empty() || source.front() != 'X') return;
  std::istringstream input{theatre.briefing.substr(1, theatre.briefing.find('\n'))};
  int angle{0}, diameter{0}, colour{0}, gravity{0}, density{0}, stars{0}, height{0};
  if(!(input >> angle >> diameter >> colour >> gravity >> density >> stars >> height)) throw std::invalid_argument{"embedded theatre has an invalid X directive"};
  theatre.sky = {
    .sun_angle{static_cast<maths::angle>(angle)},
    .first_sun_diameter{static_cast<uint16_t>(diameter)},
    .second_sun_diameter{static_cast<uint16_t>(diameter + 5)},
    .sun_colour{static_cast<uint8_t>(colour & 15)},
    .stars{(stars & 1) != 0},
  };
  theatre.environment.gravity = static_cast<int16_t>(std::clamp(gravity, -100, -15));
  if(density >= 1000 && density <= 10'000) theatre.environment.sea_level_density = static_cast<uint16_t>(density);
  if(height >= 256 && height <= 32'767) theatre.environment.atmosphere_height = static_cast<uint16_t>(height);
  theatre.briefing.erase(0, theatre.briefing.find('\n') + 1);
}

} // namespace corncob::world
