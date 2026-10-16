#include "vision.h"
#include <algorithm>
#include <cstdlib>
#include "maths/integer.h"

namespace corncob::game {

void vision_state::advance(int32_t const vertical_force, bool const on_foot, unsigned int const frame, flight::time_step const step) {
  /// DODVELAVG filters the original dword force accumulator and fades the DAC only beyond its sixty-unit load threshold
  if(static_cast<uint16_t>(frame) <= 3) return;
  int16_t const averaging_frames{maths::word(200 * step.normal / step.elapsed)};
  int32_t const difference{maths::dword(static_cast<int64_t>(on_foot ? 0 : vertical_force) - average_vertical_force)};
  average_vertical_force = maths::dword(static_cast<int64_t>(average_vertical_force) + maths::rounded_divide(difference, averaging_frames));
  int const whole{std::clamp(average_vertical_force >> 16, -100, 100)};
  average_vertical_force = maths::dword(static_cast<int64_t>(whole) * 65'536 + (static_cast<uint32_t>(average_vertical_force) & 0xffffu));
  int const overload{std::abs(whole) - 60};
  unsigned int const value{static_cast<unsigned int>(overload < 0 ? 255 : 255 - overload * 255 / 41)};
  brightness = static_cast<uint16_t>((brightness & 255u) | value << 8);
}
void vision_state::close_capture(flight::time_step const step) noexcept {
  /// The original SUB/SBB sign check permits a wrapped word; retain its fractional DAC intensity rather than clamping
  int const change{std::max(1, 60 * step.elapsed / step.normal)};
  brightness = static_cast<uint16_t>(brightness - change);
}
void vision_state::restore() noexcept {
  /// Ejection and return from intelligence restore the high intensity byte while preserving its fraction
  brightness |= 0xff00u;
}
uint8_t vision_state::intensity() const noexcept {
  /// SETCOLORS consumes only the upper byte of the stored fade word
  return static_cast<uint8_t>(brightness >> 8);
}

} // namespace corncob::game
