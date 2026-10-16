#pragma once

#include <optional>
#include "game/joystick.h"

namespace corncob::platform {

std::optional<game::joystick_sample> poll_joystick(int requested);

} // namespace corncob::platform
