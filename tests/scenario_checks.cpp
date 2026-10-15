#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include "game/actors.h"
#include "resources/assets.h"

namespace {

void require(bool const condition, char const *const message) {
  /// Stop the scenario at the first broken cross-system rule
  if(!condition) throw std::runtime_error{message};
}

} // anonymous namespace

int main() {
  /// Exercise delayed objectives, protected orbs, nearby spawning and actual hostile shell launches
  namespace world = corncob::world;
  namespace maths = corncob::maths;
  namespace game = corncob::game;
  corncob::resources::geometry_bank geometry{corncob::resources::engine_data_asset()};
  world::theatre theatre;
  auto add{[&](world::object_type const type, maths::world_position const &position, uint16_t const extra = 0){
    auto const &definition{geometry.definition(type)};
    theatre.objects.push_back({
      .position{maths::world_position{position}},
      .attitude{},
      .type{type},
      .flags{static_cast<uint16_t>(definition.flags | extra)},
      .timer{definition.initial_timer},
    });
  }};
  add(world::object_type::generator_orb, {0, 0, 6000});
  add(world::object_type::fuel_dump, {50'000, 0, 0});
  add(world::object_type::kla_site, {100'000, 0, 0});
  add(world::object_type::aaa_site, {0, 100'000, 0}, 0x7000);
  game::combat weapons{geometry, theatre};
  game::actors actors{geometry, theatre, weapons};
  maths::world_position const observer{0, 0, 5000};
  maths::fixed_vector const velocity{1000, 0, 0};
  game::actor_context const context{
    .observer{observer},
    .target{observer},
    .target_velocity{velocity},
    .on_foot{false},
    .timer{0},
  };
  actors.advance(context, {});
  require(weapons.stats().objectives_completed == 0, "intact destroy-me orb completed itself");
  world::placed_object const bomb{
    .position{0, 0, 0},
    .attitude{},
    .type{world::object_type::bomb},
    .flags{0xc9},
  };
  weapons.damage(0, bomb, true);
  actors.advance(context, {});
  require(weapons.stats().objectives_completed == 1, "destroyed orb lost its objective handler");
  actors.advance(context, {});
  require(weapons.stats().objectives_completed == 1, "completed orb repeatedly awarded credit");
  theatre.objects[2].timer = 1;
  actors.advance(context, {});
  require(std::ranges::any_of(theatre.objects, [](auto const &o){return o.type == world::object_type::kla && o.has(world::object_flag::present);}), "local KLA launcher failed to spawn");
  bool shell_seen{false};
  for(unsigned int frame{0}; frame != 300; ++frame) {
    weapons.move(observer, {});
    actors.advance(context, {});
    shell_seen = shell_seen || std::ranges::any_of(theatre.objects, [](auto const &o){return o.type == world::object_type::aa_shell && o.has(world::object_flag::present);});
    weapons.collisions(observer);
  }
  require(shell_seen, "highest-quality AAA never fired");
  require(world::in_active_neighbourhood({0, 0, 0}, observer), "observer's tile was not active");
  require(!world::in_active_neighbourhood({6'000'000, 6'000'000, 0}, observer), "distant theatre tile remained active");
  std::cout << "Local spawning, hostile shells, persistent orb objectives and four-tile activation exercised" << std::endl;
  return EXIT_SUCCESS;
}
