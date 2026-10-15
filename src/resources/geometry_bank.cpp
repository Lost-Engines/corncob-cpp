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
      .properties{input.word(0x2d44 + type * 2)},
      .collision_group{input.byte(address + 19)},
      .rotate_below_tier{input.byte(address + 26)},
      .hit_points{input.byte(address + 27)},
      .initial_timer{input.word(address + 52)},
      .initial_velocity{input.vector(address + 46)},
      .motion{
        .spin{input.vector(address + 54)},
        .dispersion{input.signed_word(address + 60)},
        .drag_divisor{input.signed_word(address + 62)},
        .gravity{input.signed_word(address + 64)},
      },
    };
    load_model(definitions[type].models.near);
    load_model(definitions[type].models.middle);
    load_model(definitions[type].models.far);
  }
  for(auto const address : {0x6f71, 0x7099, 0x5e2a, 0x3d64, 0x3d3c, 0x47a1, 0x406a, 0x4132}) load_model(static_cast<model_address>(address));
  load_model(sun_model);
  load_model(experimental_model);
}

object_definition const &geometry_bank::definition(world::object_type const type) const {
  /// Provide the original drawing, health and collision defaults for a typed world object
  return definitions.at(std::to_underlying(type));
}
model_levels geometry_bank::appearance(world::placed_object const &object) const {
  /// Typed visual states retain the model-pointer switches authored in the original object handlers
  switch(object.pose) {
  case world::model_pose::damaged:
    if(object.type == world::object_type::aircraft || object.type == world::object_type::internal_aircraft) return {0x47a1, 0x47a1, 0x4ee3};
    if(object.type == world::object_type::generator_orb) return {0x3227, 0x3227, 0x3227};
    if(object.type == world::object_type::radar_dish || object.type == world::object_type::radar_base) return {0x406a, 0x4132, 0x4132};
    if(object.type == world::object_type::fuel_dump) return {0x3521, 0x3569, 0x3569};
    break;
  case world::model_pose::gesture: return {0x6f71, 0x6f71, 0x6f71};
  case world::model_pose::shot: return {0x7099, 0x7099, 0x7099};
  case world::model_pose::paved: return {0x5e2a, 0x5e2a, 0x5ff2};
  case world::model_pose::yellow_explosion: return {0x3d64, 0x3d64, 0x3d64};
  case world::model_pose::red_explosion: return {0x3d3c, 0x3d3c, 0x3d3c};
  case world::model_pose::radar_wreck: return {0x406a, 0x4132, 0x4132};
  default: break;
  }
  return definition(object.type).models;
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
void geometry_bank::configure_sun(world::sky_parameters const &sky) {
  /// READUNIVPARAMS changes the two authored sun discs and their shared colour byte
  auto const &command{models.at(sun_model).octants.front().front()};
  auto &sun{std::get<disc_cloud>(primitives.at(command.primitive_index))};
  sun.discs.at(0).diameter = sky.first_sun_diameter;
  sun.discs.at(1).diameter = sky.second_sun_diameter;
  colours[0x537d] = sky.sun_colour;
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
