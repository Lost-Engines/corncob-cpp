#pragma once

#include <span>
#include <vector>
#include "maths/fixed.h"

namespace corncob::game {

struct recorded_frame {
  maths::world_position position{0, 0, 0};
  maths::angles attitude;
  vec3<int32_t> velocity{0, 0, 0};
  uint8_t buttons{0};
};

class recorded_flight {
private:
  std::vector<recorded_frame> motion;

public:
  explicit recorded_flight(std::span<uint8_t const> bytes);
  std::span<recorded_frame const> frames() const noexcept;
};

} // namespace corncob::game
