#pragma once

#include <span>
#include <string>
#include <vector>
#include "world/world.h"

namespace corncob::resources {

struct survey_entry {
  maths::world_position position{0, 0, 0};
  world::object_type type{world::object_type::generator_orb};
  uint16_t flags{0};
  uint8_t group{0};
};

struct airfield_status {
  unsigned int planes{0};
  unsigned int objectives{0};
};

airfield_status survey_airfield(world::theatre const &theatre, unsigned int airfield = 4);
void group_survey_entries(std::span<survey_entry> entries);
std::string describe_survey_entry(survey_entry const &entry, maths::world_position const &origin);
std::vector<std::string> airfield_survey(world::theatre const &theatre, unsigned int airfield = 4);

} // namespace corncob::resources
