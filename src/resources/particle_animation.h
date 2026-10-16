#pragma once

#include <array>
#include <optional>
#include <span>
#include "maths/fixed.h"
#include "maths/random.h"

namespace corncob::resources {

struct particle_disc {
  maths::fixed_vector position{0, 0, 0};
  uint16_t diameter{0};
};

class particle_animation {
private:
  std::array<particle_disc, 35> bases;
  std::array<particle_disc, 35> current;
  uint8_t headquarters_colour{0};

public:
  explicit particle_animation(std::span<uint8_t const> source);
  static std::optional<unsigned int> binding(uint16_t address) noexcept;
  void advance(maths::random_source &random) noexcept;
  std::span<particle_disc const> discs() const noexcept;
  uint8_t colour() const noexcept;
};

} // namespace corncob::resources
