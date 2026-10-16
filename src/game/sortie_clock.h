#pragma once

#include <cstdint>

namespace corncob::game {

class sortie_clock {
private:
  uint32_t pending_ticks{0};
  uint16_t recorded_seconds{0};

public:
  explicit sortie_clock(uint16_t recorded_seconds = 0) noexcept;
  void advance(uint32_t ticks) noexcept;
  uint16_t elapsed() const noexcept;
  void suspend() noexcept;
};

} // namespace corncob::game
