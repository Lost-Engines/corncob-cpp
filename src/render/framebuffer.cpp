#include "framebuffer.h"
#include <algorithm>
#include <cmath>

namespace corncob::render {

viewport fit_viewport(int const width, int const height) noexcept {
  /// Fit the EGA image to a 4:3 display, using integer enlargement when space permits
  if(width <= 0 || height <= 0) return {
    .x{0},
    .y{0},
    .width{0},
    .height{0},
  };
  double const available{std::min(width / static_cast<double>(screen_layout::display_width), height / static_cast<double>(screen_layout::display_height))};
  double const scale{available >= 1.0 ? std::floor(available) : available};
  int const fitted_width{std::max(1, static_cast<int>(screen_layout::display_width * scale))};
  int const fitted_height{std::max(1, static_cast<int>(screen_layout::display_height * scale))};
  return {
    .x{(width - fitted_width) / 2},
    .y{(height - fitted_height) / 2},
    .width{fitted_width},
    .height{fitted_height},
  };
}

} // namespace corncob::render
