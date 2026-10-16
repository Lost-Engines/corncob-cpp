#pragma once

#include "flight/aircraft.h"

namespace corncob::game {

struct vision_state {
  int32_t average_vertical_force{0};
  uint16_t brightness{0xff00};

  void advance(int32_t vertical_force, bool on_foot, unsigned int frame, flight::time_step step);
  void close_capture(flight::time_step step) noexcept;
  void restore() noexcept;
  uint8_t intensity() const noexcept;
};

} // namespace corncob::game
