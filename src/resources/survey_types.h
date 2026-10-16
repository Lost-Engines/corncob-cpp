#pragma once

#include <array>
#include <string_view>

namespace corncob::resources {

struct survey_type {
  std::string_view name;
  unsigned int attributes;
};

// Original MOAG 3.42 survey vocabulary and inclusion/enemy bits, file offset 0x20dac.
inline std::array<survey_type, 61> constexpr survey_types{
  survey_type{
    .name{"Headquarters"},
    .attributes{3},
  },
  survey_type{
    .name{"Mind-bender"},
    .attributes{3},
  },
  survey_type{
    .name{"Human-made plane"},
    .attributes{1},
  },
  survey_type{
    .name{"Radar"},
    .attributes{1},
  },
  survey_type{
    .name{"Radar Base"},
    .attributes{1},
  },
  survey_type{
    .name{"Runway Piece"},
    .attributes{0},
  },
  survey_type{
    .name{"Control Tower"},
    .attributes{1},
  },
  survey_type{
    .name{"Ground Crewman"},
    .attributes{1},
  },
  survey_type{
    .name{"Shard-2"},
    .attributes{0},
  },
  survey_type{
    .name{"Shard-1"},
    .attributes{0},
  },
  survey_type{
    .name{"Incendiary-blast"},
    .attributes{0},
  },
  survey_type{
    .name{"KLA-Site"},
    .attributes{3},
  },
  survey_type{
    .name{"Vehicle"},
    .attributes{3},
  },
  survey_type{
    .name{"Vehicle Site"},
    .attributes{3},
  },
  survey_type{
    .name{"Force-field"},
    .attributes{2},
  },
  survey_type{
    .name{"Force-field"},
    .attributes{2},
  },
  survey_type{
    .name{"Force-field"},
    .attributes{2},
  },
  survey_type{
    .name{"Force-field"},
    .attributes{2},
  },
  survey_type{
    .name{"Puff"},
    .attributes{0},
  },
  survey_type{
    .name{"Boom"},
    .attributes{0},
  },
  survey_type{
    .name{"Bomb"},
    .attributes{1},
  },
  survey_type{
    .name{"Human-made Missile"},
    .attributes{1},
  },
  survey_type{
    .name{"AAA Shell"},
    .attributes{3},
  },
  survey_type{
    .name{"AAA Site"},
    .attributes{3},
  },
  survey_type{
    .name{"KLA"},
    .attributes{3},
  },
  survey_type{
    .name{"Flotsm"},
    .attributes{0},
  },
  survey_type{
    .name{"Flotsm"},
    .attributes{0},
  },
  survey_type{
    .name{"Fuel Dump"},
    .attributes{3},
  },
  survey_type{
    .name{"Mystery Object"},
    .attributes{0},
  },
  survey_type{
    .name{"Propeller"},
    .attributes{0},
  },
  survey_type{
    .name{"Bullet"},
    .attributes{0},
  },
  survey_type{
    .name{"Survey Marker"},
    .attributes{0},
  },
  survey_type{
    .name{"Crater"},
    .attributes{1},
  },
  survey_type{
    .name{"Fuel Dump -- Destroyed"},
    .attributes{3},
  },
  survey_type{
    .name{"Tower Doorway"},
    .attributes{0},
  },
  survey_type{
    .name{"Runway End piece 1"},
    .attributes{0},
  },
  survey_type{
    .name{"Runway End piece 2"},
    .attributes{0},
  },
  survey_type{
    .name{"Fence"},
    .attributes{2},
  },
  survey_type{
    .name{"Fence"},
    .attributes{2},
  },
  survey_type{
    .name{"Fence"},
    .attributes{2},
  },
  survey_type{
    .name{"Barrage Balloon"},
    .attributes{3},
  },
  survey_type{
    .name{"Bumble-Bee cloud"},
    .attributes{3},
  },
  survey_type{
    .name{"Mortar"},
    .attributes{3},
  },
  survey_type{
    .name{"Headquarters -- Destroyed"},
    .attributes{3},
  },
  survey_type{
    .name{"Force-field"},
    .attributes{2},
  },
  survey_type{
    .name{"Force-field"},
    .attributes{2},
  },
  survey_type{
    .name{"Force-field"},
    .attributes{2},
  },
  survey_type{
    .name{"Force-field"},
    .attributes{2},
  },
  survey_type{
    .name{"Rescue-Van"},
    .attributes{1},
  },
  survey_type{
    .name{"Pop-Up"},
    .attributes{3},
  },
  survey_type{
    .name{"Saucer"},
    .attributes{3},
  },
  survey_type{
    .name{"Saucer-Site"},
    .attributes{3},
  },
  survey_type{
    .name{"TRFRU"},
    .attributes{3},
  },
  survey_type{
    .name{"M-O Saucer"},
    .attributes{3},
  },
  survey_type{
    .name{"Small Force-field"},
    .attributes{2},
  },
  survey_type{
    .name{"Small Force-field"},
    .attributes{2},
  },
  survey_type{
    .name{"Small Force-field"},
    .attributes{2},
  },
  survey_type{
    .name{"Alien"},
    .attributes{3},
  },
  survey_type{
    .name{"Portal"},
    .attributes{3},
  },
  survey_type{
    .name{"Small Force-field"},
    .attributes{2},
  },
  survey_type{
    .name{"Small Force-field"},
    .attributes{2},
  },
};

} // namespace corncob::resources
