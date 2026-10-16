#pragma once

#include <array>
#include "game/combat.h"
#include "maths/random.h"

namespace corncob::game {

struct actor_context {
  maths::world_position const &observer;
  maths::world_position const &target;
  maths::fixed_vector const &target_velocity;
  bool on_foot;
  uint32_t timer;
};
struct actor_effects {
  bool capture_pending{false};
  bool captured{false};
  bool capture_closing{false};
  maths::world_position approach_target{0, 0, 0};
  bool rescued{false};
  uint16_t hallucination{0};
  uint16_t orb_distortion{0};
  maths::fixed_vector attraction{0, 0, 0};
};

class actors {
private:
  resources::geometry_bank const &geometry;
  world::theatre &theatre;
  combat &weapons;
  maths::random_source random;
  std::array<unsigned int, std::to_underlying(world::object_type::count)> previous_counts{};
  unsigned int observed_aliens{0};
  actor_effects effects;
  bool census_ready{false};

  void update_launcher(world::placed_object &object, actor_context const &context, flight::time_step step);
  void update_kla(world::placed_object &object, actor_context const &context, flight::time_step step);
  void update_saucer(world::placed_object &object, actor_context const &context, flight::time_step step);
  void update_shell(unsigned int index, actor_context const &context, flight::time_step step);
  void update_aaa(world::placed_object &object, actor_context const &context, flight::time_step step);
  void update_vehicle(world::placed_object &object, actor_context const &context, flight::time_step step);
  void update_alien(world::placed_object &object, actor_context const &context, flight::time_step step);
  void update_balloon(world::placed_object &object, flight::time_step step);
  void update_orb(world::placed_object &object, actor_context const &context, flight::time_step step);
  void update_mortar(world::placed_object &object, actor_context const &context, flight::time_step step);

public:
  actors(resources::geometry_bank const &geometry, world::theatre &theatre, combat &weapons);
  void refresh_census(maths::world_position const &observer);
  actor_effects const &advance(actor_context const &context, flight::time_step step);
};

} // namespace corncob::game
