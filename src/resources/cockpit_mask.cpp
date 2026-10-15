#include "cockpit_mask.h"
#include <algorithm>
#include <stdexcept>

namespace corncob::resources {

cockpit_mask decode_cockpit_mask(std::span<uint8_t const> const source) {
  /// Match RIMG's alternating repeat/literal runs, including its destination-clipped final run
  unsigned int constexpr header_bytes{4};
  unsigned int constexpr count_mask{0b0011'1111'1111'1111u};
  if(source.size() < header_bytes) throw std::invalid_argument{"cockpit IMG header is truncated"};
  cockpit_mask output;
  size_t input{header_bytes};
  size_t written{0};
  auto read_count{[source, &input]{
    if(source.size() - input < 2) throw std::invalid_argument{"cockpit IMG run count is truncated"};
    auto const count{(static_cast<unsigned int>(source[input]) | (static_cast<unsigned int>(source[input + 1]) << 8)) & count_mask};
    input += 2;
    return count;
  }};
  while(written != output.size()) {
    auto const repeated{std::min<size_t>(read_count(), output.size() - written)};
    if(input == source.size()) throw std::invalid_argument{"cockpit IMG repeat byte is missing"};
    std::fill_n(output.data() + written, repeated, source[input++]);
    written += repeated;
    if(written == output.size()) break;
    auto const literal{std::min<size_t>(read_count(), output.size() - written)};
    if(literal > source.size() - input) throw std::invalid_argument{"cockpit IMG literal run is truncated"};
    std::copy_n(source.data() + input, literal, output.data() + written);
    input += literal;
    written += literal;
  }
  return output;
}

void draw_cockpit_preview(cockpit_mask const &mask, render::framebuffer &target) noexcept {
  /// Inspect the original monochrome resource without inventing a world or live instrument state
  for(unsigned int pixel{0}; pixel != render::screen_layout::pixel_count; ++pixel) {
    bool const foreground{(mask[pixel / 8] & (0b1000'0000u >> (pixel % 8))) != 0};
    uint8_t const shade{static_cast<uint8_t>(foreground ? 170 : 0)};
    target.pixels[pixel] = {
      .red{shade},
      .green{shade},
      .blue{shade},
    };
  }
}

} // namespace corncob::resources
