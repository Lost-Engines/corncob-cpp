#pragma once

#include "resources/geometry_bank.h"
#include "world/world.h"

namespace corncob::world {

struct training_defence {
  static unsigned int constexpr maximum_menu_count{25};
  unsigned int count;
  int quality{-1};
};
enum class training_variant {
  combat,
  slalom,
};

struct training_setup {
  training_defence kla{
    .count{5},
    .quality{-1},
  };
  training_defence vehicles{
    .count{2},
    .quality{3},
  };
  training_defence aaa{
    .count{6},
    .quality{-1},
  };
  bool invincible{true};
  training_variant variant{training_variant::combat};
};

theatre generate_slalom(resources::geometry_bank const &geometry, uint32_t seed = 2'949'243u);

theatre generate_training(resources::geometry_bank const &geometry, training_setup const &setup, uint32_t seed = 2'949'243u);

} // namespace corncob::world
