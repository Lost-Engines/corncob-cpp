#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include "graphics/raster.h"
#include "tests/reference/native_discs.h"
#include "tests/reference/native_polygons.h"

namespace {

uint64_t checksum(corncob::graphics::indexed_framebuffer const &source) {
  /// Match the reference checksum over every indexed pixel, including pixels beyond the view bounds
  uint64_t value{14'695'981'039'346'656'037ull};
  for(auto const pixel : source.pixels) value = (value ^ pixel) * 1'099'511'628'211ull;
  return value;
}

} // anonymous namespace

auto main()->int {
  /// Compare original circle sizes and viewport-edge cases with native EGA writes
  corncob::graphics::indexed_framebuffer surface;
  corncob::graphics::random_source random;
  corncob::graphics::raster draw{surface, {
    .left{8},
    .top{4},
    .right{631},
    .bottom{199},
  }, random};
  for(auto const &reference : corncob::tests::native_discs) {
    std::ranges::fill(surface.pixels, 0);
    draw.disc({reference.x, reference.y}, reference.diameter, 12);
    if(checksum(surface) != reference.hash) throw std::runtime_error{"native disc pixels differ at diameter " + std::to_string(reference.diameter)
      + " and centre " + std::to_string(reference.x) + "," + std::to_string(reference.y)};
  }
  unsigned int polygon_index{0};
  for(auto const &reference : corncob::tests::native_polygons) {
    std::ranges::fill(surface.pixels, 0);
    draw.polygon({reference.vertices.data(), reference.count}, 12);
    if(checksum(surface) != reference.hash) {
      throw std::runtime_error{"native polygon pixels differ at case " + std::to_string(polygon_index)};
    }
    ++polygon_index;
  }
  std::cout << "Native disc and polygon pixels match, including viewport clipping and degenerate shapes" << std::endl;
  return EXIT_SUCCESS;
}
