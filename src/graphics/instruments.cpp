#include "instruments.h"
#include <algorithm>
#include <array>
#include <bit>

namespace corncob::graphics {

namespace {

screen_point needle(screen_point const &centre, screen_point const &diameter, maths::angle const angle) noexcept {
  /// ANGTOCORD rounds the high product word; the stored gauge sizes are diameters rather than radii
  auto rounded{[](int const value){ return (value + 0x8000) >> 16; }};
  return {centre.x + rounded(maths::sine(angle) * diameter.x), centre.y + rounded(maths::negate(maths::cosine(angle)) * diameter.y)};
}

} // anonymous namespace

void instrument_panel::draw(indexed_framebuffer &target, random_source &random, instrument_telemetry const &telemetry, unsigned int const frame) {
  /// Recreate INST's gauges and control marks while preserving the lower three planes of the cockpit artwork
  clip_rectangle constexpr screen{
    .left{0},
    .top{0},
    .right{640},
    .bottom{350},
  };
  raster high{target, screen, random, pixel_operation::high_plane};
  raster solid{target, screen, random};
  auto const &aircraft{telemetry.aircraft};
  auto gauge{[&](screen_point const &centre, screen_point const &diameter, maths::angle const angle){
    high.line(screen_point{centre}, needle(centre, diameter, angle), 8);
  }};
  // the original spreads these four instruments over fourteen alternating video-page frames
  switch(frame % 14) {
  case 0: case 1: temperature = static_cast<maths::angle>(aircraft.engine_temperature / 3 - 5400); break;
  case 2: case 3: case 8: case 9:
    pitch = static_cast<maths::angle>(-aircraft.attitude.pitch);
    if(maths::word(aircraft.attitude.roll + 16'384) < 0) pitch = static_cast<maths::angle>(-pitch + 32'768);
    break;
  case 4: case 5: case 10: case 11: compass = static_cast<maths::angle>(-aircraft.attitude.heading); break;
  default:
    {
      unsigned int const speed{static_cast<uint16_t>(aircraft.body_velocity.x) * (aircraft.variant == flight::aircraft_variant::experimental ? 4u : 1u)};
      airspeed = static_cast<maths::angle>((speed % 19'648) * 65'536 / 19'648);
    }
    break;
  }
  int32_t const height{std::max(0, aircraft.position.z - 720)};
  uint64_t const altitude_phase{static_cast<uint64_t>(height) * 65'536 / 3050};
  gauge({43, 267}, {46, 32}, static_cast<maths::angle>(altitude_phase));
  gauge({116, 267}, {46, 32}, airspeed);
  int const climb{std::clamp(static_cast<int>(aircraft.velocity.z) + 804, 0, 1607)};
  gauge({43, 318}, {46, 32}, static_cast<maths::angle>(static_cast<unsigned int>(climb) * 65'536 / 2414 + 27'306));
  gauge({32, 232}, {40, 28}, temperature);
  gauge({127, 224}, {20, 14}, compass);
  gauge({116, 318}, {10, 7}, aircraft.attitude.roll);
  auto horizon{[&](screen_point const &centre, screen_point const &diameter, maths::angle const angle){
    auto const end{needle(centre, diameter, static_cast<maths::angle>(angle + 16'384))};
    screen_point const opposite{centre * 2 - end};
    high.line(screen_point{opposite}, screen_point{end}, 8);
  }};
  horizon({116, 318}, {46, 32}, aircraft.attitude.roll);
  horizon({174, 224}, {30, 21}, pitch);
  auto const pitch_end{needle({174, 224}, {30, 21}, static_cast<maths::angle>(pitch + 16'384))};
  gauge(pitch_end, {10, 7}, pitch);
  solid.line({79, 224}, needle({79, 224}, {20, 14}, static_cast<maths::angle>((aircraft.oil_pressure / 3) * 4 - (32'767 - 5400))), 13);
  solid.line({43, 267}, needle({43, 267}, {28, 20}, static_cast<maths::angle>(altitude_phase / 10)), 9);
  solid.line({43, 267}, needle({43, 267}, {24, 16}, static_cast<maths::angle>(altitude_phase / 100)), 7);
  high.disc({static_cast<int>(aircraft.throttle) * 148 / 200 + 478, 246}, 5, 8);
  high.line({385 + (telemetry.controls.roll + 512) * 44 / 1024, 230}, {385 + (telemetry.controls.roll + 512) * 44 / 1024, 234}, 8);
  high.line({385 + (-telemetry.controls.rudder + 512) * 44 / 1024, 214}, {385 + (-telemetry.controls.rudder + 512) * 44 / 1024, 218}, 8);
  high.line({365, 212 + (-telemetry.controls.pitch + 512) * 25 / 1024}, {371, 212 + (-telemetry.controls.pitch + 512) * 25 / 1024}, 8);
  int const flow_pitch{std::clamp(-static_cast<int>(std::bit_cast<int16_t>(maths::arctangent(aircraft.body_velocity.x, aircraft.body_velocity.z))) + 5400, 0, 10'799)};
  int const flow_yaw{std::clamp(static_cast<int>(std::bit_cast<int16_t>(maths::arctangent(aircraft.body_velocity.x, aircraft.body_velocity.y))) + 5400, 0, 10'799)};
  solid.disc({559 + flow_yaw * 160 / 10'800 - 80, 298 + flow_pitch * 110 / 10'800 - 55}, 6,
             aircraft.variant == flight::aircraft_variant::reserve ? 9 : 12);
  if(telemetry.damage != nullptr) {
    std::array<uint8_t, 13> constexpr colours{10, 10, 9, 9, 9, 9, 14, 14, 14, 14, 12, 12, 8};
    std::array<int, 4> constexpr rows{213, 219, 226, 233};
    for(unsigned int channel{0}; channel != rows.size(); ++channel) {
      auto const hits{telemetry.damage->indicator_hits[channel]};
      for(unsigned int bar{0}; bar < std::min<unsigned int>(hits, 13); ++bar) {
        uint8_t colour{colours[bar]};
        if(bar == 12 && ((frame * 40 / (hits > 14 ? 128 : hits > 13 ? 256 : 512)) & 1u) != 0) colour = 0;
        int const x{536 + static_cast<int>(bar) * 6};
        solid.line({x, rows[channel]}, {x + 5, rows[channel]}, colour);
      }
    }
  }
  auto button{[&](clip_rectangle const region, uint8_t const foreground, uint8_t const background){
    for(int y{region.top}; y <= region.bottom; ++y) {
      for(int x{region.left}; x < region.right; ++x) {
        auto &pixel{target.pixels[static_cast<unsigned int>(y) * 640 + static_cast<unsigned int>(x)]};
        pixel = pixel == 7 ? foreground : pixel == 0 ? background : pixel;
      }
    }
  }};
  button({205, 209, 248, 218}, telemetry.autopilot ? 15 : 7, telemetry.autopilot ? 2 : 0);
  button({205, 219, 248, 228}, telemetry.paused ? 15 : 7, telemetry.paused ? 5 : 0);
  button({205, 229, 248, 241}, aircraft.flaps ? 15 : 7, aircraft.flaps ? 6 : 0);
  button({443, 209, 484, 219}, aircraft.stalled ? 14 : 7, aircraft.stalled ? 12 : 0);
  button({443, 231, 484, 241}, telemetry.completed ? 15 : 7, telemetry.completed ? 2 : 0);
}

} // namespace corncob::graphics
