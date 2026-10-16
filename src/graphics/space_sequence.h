#pragma once

#include <array>
#include "graphics/raster.h"

namespace corncob::graphics {

struct space_star {
  int16_t x;
  int16_t y;
  uint16_t z;
};

class star_field {
private:
  std::array<space_star, 768> stars;
  random_source random;

public:
  explicit star_field(uint32_t seed = 2'949'243u);
  void restart();
  void advance(indexed_framebuffer &target);
  std::span<space_star const> positions() const noexcept;
};

class space_sequence {
private:
  std::array<indexed_framebuffer, 2> pages;
  std::array<render::rgba_pixel, 16> palette;
  star_field stars;
  random_source random;
  unsigned int display_page{0};
  unsigned int frame{0};
  unsigned int warp_frames{0};
  bool warp{false};
  bool leaving{false};
  bool complete{false};
  double clock{0.0};

  void tick();

public:
  static double constexpr tick_period{65'536.0 / 1'193'182.0};
  space_sequence(indexed_framebuffer const &backdrop, std::array<render::rgba_pixel, 16> const &colours);
  void advance(double seconds);
  void toggle_warp() noexcept;
  bool finished() const noexcept;
  void render(render::framebuffer &target) const;
};

} // namespace corncob::graphics
