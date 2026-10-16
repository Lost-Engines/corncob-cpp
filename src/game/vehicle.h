#pragma once

#include "maths/random.h"
#include "world/world.h"

namespace corncob::game {

struct vehicle_effects {
  bool capture_pending{false};
  bool capture_closing{false};
  bool rescued{false};
  maths::world_position approach_target{0, 0, 0};
};

unsigned int vehicle_tier(world::placed_object const &object, maths::world_position const &observer) noexcept;
void follow_vehicle(world::placed_object &object, unsigned int tier);
void reset_vehicle(world::placed_object &object, maths::world_position const &observer, unsigned int tier, bool on_foot, maths::random_source &random, vehicle_effects &effects);

} // namespace corncob::game
