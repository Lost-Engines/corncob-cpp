#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <utility>
#include <vector>
#include "maths/fixed.h"

namespace corncob::world {

enum class object_type : uint16_t {
  generator_orb, mindbender, aircraft, radar_dish, radar_base,
  runway_piece, control_tower, ground_crew, shard_2, shard,
  incendiary_fragment, kla_site, ground_vehicle, vehicle_site,
  forcefield_z1, forcefield_x, forcefield_y, forcefield_z,
  puff, explosion, bomb, missile, aa_shell, aaa_site, kla,
  flotsam_1, flotsam_2, fuel_dump, internal_aircraft, propeller,
  bullets, survey_marker, crater, destroyed_fuel_dump, tower_doorway,
  runway_end, runway_end_1, fence_1, fence_1a, fence_1b,
  barrage_balloon, bee_swarm, mortar, destroyed_generator_orb,
  wire_forcefield_z1, wire_forcefield_x, wire_forcefield_y, wire_forcefield_z,
  rescue_van, shell, saucer, saucer_site, tet, objective_saucer,
  small_wire_forcefield_x, small_wire_forcefield_y, small_wire_forcefield_z,
  alien, portal, small_step_forcefield, filled_small_step_forcefield,
  count,
};

enum class object_flag : uint16_t {
  present = 0x0001,
  collision_callback = 0x0008,
  updates = 0x0010,
  player_contact = 0x0020,
  six_hit_attack = 0x0040,
  heavy_attack = 0x0080,
  box_collision = 0x0100,
  objective_complete = 0x0200,
  special = 0x0800,
};

struct placed_object {
  maths::world_position position{0, 0, 0};
  maths::angles attitude;
  maths::fixed_vector velocity{0, 0, 0};
  object_type type{object_type::generator_orb};
  uint16_t flags{0};
  uint16_t timer{0};

  unsigned int quality() const noexcept {
    /// Qualities occupy three stored flag bits and index eight-row tuning tables
    return (flags >> 12) & 7u;
  }
  bool has(object_flag const flag) const noexcept {
    /// Common flag meanings apply independently of type-specific overloaded state bits
    return (flags & std::to_underlying(flag)) != 0;
  }
};

struct world_tile {
  unsigned int first_object{0};
  unsigned int object_count{0};
  uint8_t storage_flags{0};
  std::array<uint8_t, 5> packed_colours;
};

struct theatre {
  static unsigned int constexpr columns{8};
  static unsigned int constexpr rows{8};
  static int32_t constexpr tile_size{32 * 65'536};
  static int32_t constexpr origin{-128 * 65'536};
  std::array<world_tile, columns * rows> tiles;
  std::vector<placed_object> objects;
  maths::world_position start{0, 0, 0};
};

theatre decode_theatre(std::span<uint8_t const> source);

} // namespace corncob::world
