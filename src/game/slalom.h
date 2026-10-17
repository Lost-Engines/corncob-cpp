#pragma once

#include <cstdint>
#include "maths/fixed.h"

namespace corncob::game {

class slalom_course {
private:
  int32_t previous_x;
  unsigned int passed{0};
  bool awarded{false};

public:
  static unsigned int constexpr marker_count{6};
  static int32_t constexpr marker_spacing{131'072};
  static int32_t constexpr centre_y{131'072};

  explicit slalom_course(int32_t start_x) : previous_x{start_x} {}
  bool advance(maths::world_position const &position, bool on_ground, bool at_home) noexcept;
  unsigned int progress() const noexcept {
    return passed;
  }
};

} // namespace corncob::game
