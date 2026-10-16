#pragma once

#include "maths/fixed.h"

namespace corncob::resources {

enum class model_pulse {
  wide_wall,
  small_wall,
  square_wall,
  upper_saucer,
  lower_saucer,
};

struct model_animation {
  int16_t wide_wall;
  int16_t small_wall;
  int16_t square_wall;
  uint16_t upper_saucer;
  uint16_t lower_saucer;

  static model_animation at(unsigned int frame) noexcept;
  int16_t value(model_pulse pulse) const noexcept;
};

} // namespace corncob::resources
