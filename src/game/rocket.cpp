#include "rocket.h"
#include "maths/fixed.h"

namespace corncob::game {

bool rocket_charge::advance(flight::aircraft_state &aircraft, uint16_t const tick, uint16_t const elapsed, uint16_t const actual_elapsed) noexcept {
  /// Original recharge uses BIOS ticks and time-advance correction; a full/overflowing sum leaves the stored charge unchanged
  colour = aircraft.rocket ? 12 : 1;
  uint16_t const delta{static_cast<uint16_t>(tick - previous_tick)};
  previous_tick = tick;
  uint32_t const product{static_cast<uint32_t>(delta) * elapsed};
  if((product >> 16) >= actual_elapsed) return false;
  uint16_t const charged{static_cast<uint16_t>(product / actual_elapsed + static_cast<uint16_t>(aircraft.rocket_ticks))};
  if(charged > capacity) {
    colour = 10;
    return true;
  }
  aircraft.rocket_ticks = maths::word(charged);
  return false;
}

} // namespace corncob::game
