#pragma once

#include <cstdint>
#include <span>
#include "vectorstorm/vector/vector2.h"
#include "render/framebuffer.h"

namespace corncob::graphics {

using screen_point = vec2<int>;
using colour_index = uint8_t;

struct clip_rectangle {
  int left;
  int top;
  int right;
  int bottom;
};

struct indexed_framebuffer {
  std::array<colour_index, render::screen_layout::pixel_count> pixels;
};

class random_source {
private:
  uint32_t state{2'949'243u};

public:
  uint16_t next() noexcept {
    /// Retain the assembly engine's wrapping multiplicative random stream
    state *= 0x27'8d'de'6du;
    return static_cast<uint16_t>(state >> 16);
  }
};

class raster {
private:
  indexed_framebuffer &target;
  clip_rectangle bounds;
  random_source &random;

  void raw_pixel(int x, int y, colour_index colour) noexcept;
  void span(int y, int left, int right, colour_index colour, uint8_t mask = 0xff) noexcept;

public:
  raster(indexed_framebuffer &target, clip_rectangle bounds, random_source &random);
  void point(screen_point const &position, colour_index colour) noexcept;
  void line(screen_point start, screen_point end, colour_index colour) noexcept;
  void polygon(std::span<screen_point const> vertices, colour_index colour) noexcept;
  void disc(screen_point const &centre, int diameter, colour_index colour) noexcept;
};

} // namespace corncob::graphics
