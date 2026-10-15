#pragma once

#include <cstdint>
#include "flight/controls.h"
#include "flight/damage.h"
#include "graphics/raster.h"

namespace corncob::graphics {

struct instrument_telemetry {
  flight::aircraft_state const &aircraft;
  flight::control_input controls;
  bool paused{false};
  bool completed{false};
  flight::damage_state const *damage{nullptr};
};

class instrument_panel {
private:
  maths::angle airspeed{0};
  maths::angle temperature{0};
  maths::angle compass{0};
  maths::angle pitch{0};

public:
  void draw(indexed_framebuffer &target, random_source &random, instrument_telemetry const &telemetry, unsigned int frame);
};

} // namespace corncob::graphics
