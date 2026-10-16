#include "airfield_survey.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <format>
#include <numbers>

namespace corncob::resources {

namespace {

std::string_view object_name(world::placed_object const &object) {
  /// Match the original intelligence vocabulary while omitting scenery that has no intelligence entry
  using enum world::object_type;
  switch(object.type) {
  case aircraft: return (object.flags & 0x2000u) != 0 ? "Human-made plane (Stealth)" : "Human-made plane (Corsair)";
  case radar_dish: return "Radar";
  case radar_base: return "Radar Base";
  case control_tower: return "Control Tower";
  case ground_crew: return "Ground Crewman";
  case generator_orb: case destroyed_generator_orb: return "Enemy Headquarters";
  case mindbender: return "Enemy Mindbender";
  case kla_site: return "Enemy KLA-Site";
  case ground_vehicle: return "Enemy Vehicle (bumpers)";
  case vehicle_site: return "Enemy Vehicle Site";
  case aaa_site: return "Enemy AAA Site";
  case kla: return "Enemy KLA";
  case fuel_dump: return "Enemy Fuel Dump";
  case destroyed_fuel_dump: return "Enemy Fuel Dump (Destroyed)";
  case bee_swarm: return "Enemy Bumble-Bee cloud";
  case barrage_balloon: return "Barrage Balloon";
  case rescue_van: return "Human-made Rescue Vehicle";
  case saucer: return "Enemy Saucer";
  case objective_saucer: return "Enemy M-O Saucer";
  case portal: return "Alien Portal";
  default: return {};
  }
}
bool typed(world::object_type const type) {
  /// Defences expose their authored one-to-eight quality; human structures and fuel dumps have no quality label
  using enum world::object_type;
  return type == kla_site || type == ground_vehicle || type == vehicle_site || type == aaa_site || type == kla || type == objective_saucer;
}

} // anonymous namespace

std::vector<std::string> airfield_survey(world::theatre const &theatre) {
  /// Rebuild the live survey rather than showing the unrelated played-install TOWERS.INF bundled with the research input
  double constexpr centimetres_per_mile{160'934.4}, centimetres_per_foot{30.48};
  std::array<std::string_view, 16> constexpr directions{"N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE", "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"};
  std::vector<world::placed_object const*> objects;
  for(auto const &object : theatre.objects)
    if(object.has(world::object_flag::present) && !object_name(object).empty()) objects.push_back(&object);
  auto range{[&](world::placed_object const &object){
    return std::hypot(static_cast<double>(object.position.x) - theatre.start.x, static_cast<double>(object.position.y) - theatre.start.y);
  }};
  std::ranges::stable_sort(objects, [&](auto const a, auto const b){
    bool const friendly_a{a->type == world::object_type::aircraft || a->type == world::object_type::radar_dish || a->type == world::object_type::radar_base || a->type == world::object_type::control_tower || a->type == world::object_type::ground_crew};
    bool const friendly_b{b->type == world::object_type::aircraft || b->type == world::object_type::radar_dish || b->type == world::object_type::radar_base || b->type == world::object_type::control_tower || b->type == world::object_type::ground_crew};
    if(friendly_a != friendly_b) return friendly_a;
    if(friendly_a && a->type != b->type) return a->type < b->type;
    return range(*a) < range(*b);
  });
  std::vector<std::string> result;
  for(auto const *object : objects) {
    unsigned int const tenths{static_cast<unsigned int>(range(*object) * 10 / centimetres_per_mile)};
    double const direction{std::atan2(static_cast<double>(theatre.start.y) - object->position.y, static_cast<double>(object->position.x) - theatre.start.x)};
    auto const bearing{static_cast<unsigned int>(static_cast<int>(std::lround(direction * 8 / std::numbers::pi)) & 15)};
    std::string const distance{(tenths < 10 ? std::string{} : std::to_string(tenths / 10)) + "." + std::to_string(tenths % 10)};
    result.push_back(std::format("{:<27} {:7} {:>5} {:>3}   {:>6}", object_name(*object), typed(object->type) ? "Type " + std::to_string(object->quality() + 1) : "",
      distance, directions[bearing], static_cast<int64_t>(object->position.z / centimetres_per_foot)));
  }
  return result;
}

} // namespace corncob::resources
