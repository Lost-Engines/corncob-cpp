#include "geometry_bank.h"
#include <algorithm>
#include <stdexcept>
#include <utility>
#include "resources/binary_reader.h"
#include "resources/colour_rules.h"

namespace corncob::resources {

unsigned int geometry_bank::load_primitive(unsigned int const kind, model_address const address) {
  /// Cache typed drawing payloads once while preserving their original shared addresses
  uint32_t const key{(kind << 16) | address};
  if(auto const found{primitive_ids.find(key)}; found != primitive_ids.end()) return found->second;
  binary_reader const input{source};
  unsigned int const count{input.word(address)};
  if(count > 4096) throw std::invalid_argument{"original model has an invalid primitive count"};
  unsigned int const index{static_cast<unsigned int>(primitives.size())};
  switch(kind) {
  case 8:
    {
      polygon shape{
        .vertices{},
        .colour{input.word(address + 2)},
      };
      for(unsigned int i{0}; i != count; ++i) shape.vertices.push_back(input.vector(address + 4 + i * 6));
      primitives.emplace_back(std::move(shape));
    }
    break;
  case 2:
    {
      wireframe shape;
      for(unsigned int i{0}; i != count; ++i) {
        unsigned int const line_address{input.word(address + 2 + i * 2)};
        unsigned int const segments{input.word(line_address)};
        if(segments > 4096) throw std::invalid_argument{"original model has an invalid polyline length"};
        polyline line{
          .vertices{},
          .colour{input.word(line_address + 2)},
        };
        for(unsigned int vertex{0}; vertex != segments + 1; ++vertex) line.vertices.push_back(input.vector(line_address + 4 + vertex * 6));
        shape.lines.push_back(std::move(line));
      }
      primitives.emplace_back(std::move(shape));
    }
    break;
  case 4:
    {
      point_cloud shape;
      for(unsigned int i{0}; i != count; ++i) {
        unsigned int const point_address{address + 2 + i * 8};
        shape.points.push_back({
          .position{input.vector(point_address)},
          .colour{input.word(point_address + 6)},
        });
      }
      primitives.emplace_back(std::move(shape));
    }
    break;
  case 6:
    {
      disc_cloud shape;
      for(unsigned int i{0}; i != count; ++i) {
        unsigned int const disc_address{address + 2 + i * 10};
        shape.discs.push_back({
          .position{input.vector(disc_address)},
          .diameter{input.word(disc_address + 6)},
          .colour{input.word(disc_address + 8)},
        });
      }
      primitives.emplace_back(std::move(shape));
    }
    break;
  default:
    throw std::invalid_argument{"original model has an unsupported drawing opcode"};
  }
  primitive_ids.emplace(key, index);
  return index;
}

void geometry_bank::load_model(model_address const address) {
  /// Retain all eight authored painter lists, including repeated and direction-specific commands
  if(models.contains(address)) return;
  binary_reader const input{source};
  model result;
  unsigned int const count{address == 0 ? 0u : input.word(address + 6)};
  if(count > 256) throw std::invalid_argument{"original model has an invalid command count"};
  for(unsigned int octant{0}; octant != result.octants.size(); ++octant) {
    for(unsigned int i{0}; i != count; ++i) {
      unsigned int const command_address{address + 8 + (octant * count + i) * 4};
      unsigned int const opcode{input.word(command_address)};
      if(opcode == 0) continue;
      result.octants[octant].push_back({
        .primitive_index{load_primitive(opcode, input.word(command_address + 2))},
      });
    }
  }
  models.emplace(address, std::move(result));
}

geometry_bank::geometry_bank(std::span<uint8_t const> const source) :
  source{source} {
  /// Decode the original data segment as resources; no original instructions are interpreted
  unsigned int constexpr template_table{0x2cca};
  binary_reader const input{source};
  if(source.size() > colours.size()) throw std::invalid_argument{"original model data exceeds its native address space"};
  std::ranges::fill(colours, 0);
  std::ranges::copy(source, colours.begin());
  for(unsigned int type{0}; type != definitions.size(); ++type) {
    unsigned int const address{input.word(template_table + type * 2)};
    if(input.word(address + 72) != type) throw std::invalid_argument{"original object template failed its type check"};
    definitions[type] = {
      .models{
        .near{input.word(address + 40)},
        .middle{input.word(address + 42)},
        .far{input.word(address + 44)},
      },
      .limits{
        .near_squared{input.dword(address + 28)},
        .middle_squared{input.dword(address + 32)},
        .far_squared{input.dword(address + 36)},
      },
      .collision_extents{input.vector(address + 66)},
      .flags{input.word(address + 24)},
      .rotate_below_tier{input.byte(address + 26)},
      .hit_points{input.byte(address + 27)},
    };
    load_model(definitions[type].models.near);
    load_model(definitions[type].models.middle);
    load_model(definitions[type].models.far);
  }
  load_model(sun_model);
  load_model(experimental_model);
}

object_definition const &geometry_bank::definition(world::object_type const type) const {
  /// Provide the original drawing, health and collision defaults for a typed world object
  return definitions.at(std::to_underlying(type));
}
std::span<draw_command const> geometry_bank::commands(model_address const address, unsigned int const octant) const {
  /// Expose the selected painter list without copying or sorting its primitives
  return models.at(address).octants.at(octant);
}
primitive const &geometry_bank::drawing(unsigned int const index) const {
  /// Access one already decoded primitive shared by the authored command lists
  return primitives.at(index);
}
uint8_t geometry_bank::colour(colour_reference const reference) const noexcept {
  /// A nonzero high byte marks a native colour-variable address rather than a literal index
  return (reference >> 8) != 0 ? colours[reference] : static_cast<uint8_t>(reference);
}
void geometry_bank::prepare_colours(world::placed_object const &object, unsigned int const frame, uint16_t const timer, uint8_t const random, bool const radar_active) {
  /// Apply the traced per-object quality, shared random and timer-based colour bindings
  uint64_t const type_bit{uint64_t{1} << std::to_underlying(object.type)};
  for(auto const &rule : colour_rules) {
    if(rule.types != 0 && (rule.types & type_bit) == 0) continue;
    unsigned int value{0};
    switch(rule.kind) {
    case colour_rule_kind::random:
      value = random;
      break;
    case colour_rule_kind::alternate:
      value = rule.values[frame & 1u];
      break;
    case colour_rule_kind::radar:
      value = rule.values[radar_active && (timer & rule.timer_mask) != 0 ? 1 : 0];
      break;
    case colour_rule_kind::flag_blink:
      value = rule.values[(object.flags & rule.flag_mask) != 0 && (timer & rule.timer_mask) != 0 ? 1 : 0];
      break;
    case colour_rule_kind::quality:
    case colour_rule_kind::quality_random:
      value = rule.values[object.quality()];
      if((rule.kind == colour_rule_kind::quality_random && value >= 0x8000u) || (rule.random_types & type_bit) != 0) value = random;
      break;
    }
    colours[rule.address] = static_cast<uint8_t>(value);
  }
}

} // namespace corncob::resources
