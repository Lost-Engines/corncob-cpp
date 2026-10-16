#pragma once

#include "resources/geometry_bank.h"
#include "world/world.h"

namespace corncob::world {

struct training_defence {
  unsigned int count;
  int quality{-1};
};
struct training_setup {
  training_defence kla{
    .count{8},
    .quality{-1},
  };
  training_defence vehicles{
    .count{8},
    .quality{-1},
  };
  training_defence aaa{
    .count{6},
    .quality{-1},
  };
  bool invincible{false};
};

theatre generate_training(resources::geometry_bank const &geometry, training_setup const &setup, uint32_t seed = 2'949'243u);

} // namespace corncob::world
