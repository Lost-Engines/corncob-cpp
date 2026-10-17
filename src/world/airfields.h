#pragma once

#include <array>
#include <string_view>
#include "world/world.h"

namespace corncob::world {

inline unsigned int constexpr central_airfield{4}, airfield_count{9};
inline std::array<std::string_view, airfield_count> constexpr airfield_names{
  "Henderson Field", "Dulles International Airport", "Bubba's Airstrip",
  "Weedsport", "RDU ^international Airport", "Crickety Point Landing Strip",
  "Sweetly Bending Oak Aircraft Park", "Easter Island Municipal Air Travel Center", "Valmart Airstrip and Family Value Center",
};

bool multiple_airfields(theatre const &world) noexcept;
unsigned int airfield_for(maths::world_position const &position) noexcept;
unsigned int nearest_airfield(theatre const &world, maths::world_position const &position) noexcept;
bool belongs_to_airfield(theatre const &world, placed_object const &object, unsigned int airfield) noexcept;
maths::world_position airfield_start(theatre const &world, unsigned int airfield);
void select_airfield(theatre &world, unsigned int airfield);

} // namespace corncob::world
