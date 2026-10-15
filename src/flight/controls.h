#pragma once

#include <cstdint>
#include "flight/aircraft.h"

namespace corncob::flight {

struct control_input {
  int16_t roll{0};
  int16_t pitch{0};
  int16_t rudder{0};
  bool brakes{false};
};

struct angular_response {
  int32_t roll{0};
  int32_t pitch{0};
  int32_t yaw{0};
  int16_t wing_damage{0};
  int16_t rudder_damage{0};
  uint16_t almost_stall_angle{2548};
  uint16_t last_protected_pitch_rate{0};
  bool stall_protection{false};
};

void advance_controls(aircraft_state &aircraft, angular_response &response, control_input const &input,
                      atmosphere const &environment, time_step step);

} // namespace corncob::flight
