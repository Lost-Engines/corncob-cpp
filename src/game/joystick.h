#pragma once

#include "game/session.h"

namespace corncob::game {

struct joystick_sample {
  vec2<int16_t> axes{0, 0};
  bool gun{false};
  bool missile{false};
};

int16_t joystick_axis(int16_t displacement) noexcept;
void apply_joystick(input_state &input, joystick_sample const &sample, bool keypad_look) noexcept;

} // namespace corncob::game
