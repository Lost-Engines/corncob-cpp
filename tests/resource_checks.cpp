#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include "render/framebuffer.h"
#include "resources/assets.h"
#include "resources/cockpit_mask.h"

namespace {

void require(bool const condition, char const *const message) {
  /// Keep reference checks active in Release as well as assertion-enabled builds
  if(!condition) throw std::runtime_error{message};
}

} // anonymous namespace

auto main()->int {
  /// Compare decompression with the mask already verified against the native RIMG routine
  auto const decoded{corncob::resources::decode_cockpit_mask(corncob::resources::cockpit_asset())};
  uint64_t hash{14'695'981'039'346'656'037ull};
  uint64_t constexpr fnv_prime{1'099'511'628'211ull};
  for(auto const byte : decoded) hash = (hash ^ byte) * fnv_prime;
  require(hash == 0x4f'f0'eb'ad'7c'69'b6'47ull, "embedded cockpit differs from the native-verified 28,000-byte mask");

  // a repeat can fill the destination before the stream supplies another literal count
  std::array<uint8_t, 14> const final_repeat{0, 0, 0, 0, 0xff, 0xff, 0xa5, 0, 0, 0xff, 0xff, 0xa5, 0, 0};
  auto const repeated{corncob::resources::decode_cockpit_mask(final_repeat)};
  require(std::ranges::all_of(repeated, [](uint8_t const value){
    return value == 0xa5;
  }), "destination-clipped final repeat was not decoded");

  bool rejected{false};
  try {
    corncob::resources::decode_cockpit_mask(std::array<uint8_t, 4>{0, 0, 0, 0});
  } catch(std::invalid_argument const&) {
    rejected = true;
  }
  require(rejected, "truncated cockpit IMG was accepted");
  auto const desktop{corncob::render::fit_viewport(1920, 1080)};
  require(desktop.width == 1280 && desktop.height == 960 && desktop.x == 320 && desktop.y == 60, "integer display scaling does not fit the desktop");
  auto const small{corncob::render::fit_viewport(320, 240)};
  require(small.width == 320 && small.height == 240, "small-window downscaling changed the display aspect ratio");
  std::cout << "Embedded cockpit matches the native-verified mask; boundary and display checks pass" << std::endl;
  return EXIT_SUCCESS;
}
