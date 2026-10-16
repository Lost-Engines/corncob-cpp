#pragma once

#include <cstdint>
#include "flight/aircraft.h"

namespace corncob::game {

struct rocket_charge {
  static uint16_t constexpr capacity{flight::aircraft_state::rocket_capacity};
  uint16_t previous_tick{0};
  uint8_t colour{13};

  bool advance(flight::aircraft_state &aircraft, uint16_t tick, uint16_t elapsed, uint16_t actual_elapsed) noexcept;
};

} // namespace corncob::game
