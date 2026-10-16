#pragma once

#include <cstdint>
#include "maths/fixed.h"
#include "maths/random.h"

namespace corncob::game {

enum class portal_transfer : int8_t {
  none,
  home,
  random = -1,
};

void transfer_portal(maths::world_position &position, maths::world_position const &start, portal_transfer transfer, maths::random_source &random) noexcept;

} // namespace corncob::game
