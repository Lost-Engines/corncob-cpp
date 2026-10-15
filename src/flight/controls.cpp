#include "controls.h"
#include <algorithm>
#include <bit>
#include <cstdlib>
#include "maths/integer.h"

namespace corncob::flight {

namespace {

int32_t add(int32_t const a, int32_t const b) noexcept {
  /// Preserve the original low/high-word ADD/ADC pair
  return maths::dword(static_cast<int64_t>(a) + b);
}
int16_t high(int32_t const value) noexcept {
  /// Angular velocity is the signed upper word of its accumulated response
  return static_cast<int16_t>(value >> 16);
}
int32_t replace_high(int32_t const value, int16_t const replacement) noexcept {
  /// Limiting angular speed retains the low fractional word
  return std::bit_cast<int32_t>((static_cast<uint32_t>(static_cast<uint16_t>(replacement)) << 16) | (static_cast<uint32_t>(value) & 0xffffu));
}
int32_t wide_word(int16_t const value) noexcept {
  /// CWD/XCHG preserves a sign-filled fractional word for negative response biases
  return maths::dword(static_cast<int64_t>(value) * 65'536 + (value < 0 ? 65'535 : 0));
}

} // anonymous namespace

void advance_controls(aircraft_state &aircraft, angular_response &response, control_input const &input,
                      atmosphere const &environment, time_step const step) {
  /// Apply original stick torque, inertia, weathercock yaw, ground pitch and stall protection
  int16_t constexpr control_airflow_limit{300};
  int16_t constexpr roll_scale{600}, pitch_scale{125}, yaw_scale{125};
  int16_t constexpr roll_inertia{8}, pitch_inertia{10}, yaw_inertia{10};
  int16_t constexpr weathercock_scale{1053}, bank_turn_scale{3859};
  int16_t constexpr resting_pitch{-1500}, rotation_speed{2821}, maximum_rate{3000};
  int16_t const airflow{std::clamp<int16_t>(aircraft.scaled_body_velocity.x, -control_airflow_limit, control_airflow_limit)};
  auto pressure_adjust{[&](int16_t const value){
    return maths::word(static_cast<int32_t>(aircraft.density) * value / environment.sea_level_density);
  }};
  int32_t roll_torque{static_cast<int32_t>(pressure_adjust(input.roll)) * airflow};
  roll_torque = maths::scale(roll_torque, airflow) / control_airflow_limit;
  roll_torque = maths::scale(roll_torque, roll_scale);
  int32_t const roll_damping{maths::dword(static_cast<int64_t>(response.roll) * 3) >> 1};
  response.roll = add(response.roll, maths::rounded_divide(add(roll_torque, -roll_damping), roll_inertia));
  if(response.wing_damage != 0) response.roll = add(response.roll, maths::scale(wide_word(response.wing_damage) >> 3, airflow) / control_airflow_limit);

  // the original large-sideslip branch swaps a small signed value into the high response word
  int16_t const sideslip{std::bit_cast<int16_t>(maths::arctangent(aircraft.body_velocity.x, aircraft.body_velocity.y))};
  int32_t correction{0};
  if(sideslip < -150 || sideslip > 150) {
    int16_t const limited{std::clamp<int16_t>(sideslip, -150, 150)};
    int32_t const shifted{static_cast<int32_t>(limited) >> 4};
    correction = std::bit_cast<int32_t>((static_cast<uint32_t>(static_cast<uint16_t>(shifted)) << 16)
      | static_cast<uint16_t>(limited < 0 ? -1 : 0));
  }
  int16_t const rudder_scale{static_cast<int16_t>(aircraft.on_ground && static_cast<uint16_t>(aircraft.body_velocity.x) <= 1500 ? yaw_scale * 4 : yaw_scale)};
  int32_t yaw_torque{maths::scale(static_cast<int32_t>(airflow) * input.rudder, rudder_scale)};
  yaw_torque = add(add(yaw_torque, -response.yaw), correction);
  yaw_torque = add(yaw_torque, static_cast<int32_t>(maths::negate(aircraft.side_force)) * weathercock_scale);
  response.yaw = add(response.yaw, maths::rounded_divide(yaw_torque, yaw_inertia));
  if(response.rudder_damage != 0) response.yaw = add(response.yaw, maths::scale(wide_word(response.rudder_damage) >> 3, airflow) / control_airflow_limit);
  if(aircraft.scaled_body_velocity.x >= 30) {
    int16_t const bank_force{maths::multiply(maths::multiply(aircraft.world_to_body.rows[1].z, aircraft.world_to_body.rows[2].z), bank_turn_scale)};
    response.yaw = add(response.yaw, maths::rounded_divide(static_cast<int32_t>(bank_force) * 65'536, maths::negate(aircraft.scaled_body_velocity.x)));
  }
  int32_t pitch_torque{static_cast<int32_t>(pressure_adjust(maths::word(input.pitch + 5))) * airflow};
  pitch_torque = maths::scale(pitch_torque, pitch_scale);
  response.pitch = add(response.pitch, maths::rounded_divide(add(pitch_torque, -response.pitch), pitch_inertia));

  int16_t pitch{std::bit_cast<int16_t>(aircraft.attitude.pitch)};
  if(aircraft.on_ground) {
    int const speed{std::abs(static_cast<int>(aircraft.body_velocity.x))};
    if(speed <= rotation_speed) {
      int16_t const error{maths::word(pitch - resting_pitch)};
      // this byte shuffle differs from a regular fixed-point left shift
      uint32_t const shuffled{(static_cast<uint32_t>(error < 0 ? 0xffu : 0u) << 24)
        | ((static_cast<uint32_t>(static_cast<uint16_t>(error)) << 8) & 0x00ffff00u)
        | (static_cast<uint16_t>(error) & 0xffu)};
      response.pitch = add(response.pitch, maths::rounded_divide(maths::scale(std::bit_cast<int32_t>(shuffled), step.elapsed), step.normal));
    } else response.pitch = add(response.pitch, -(speed - rotation_speed) * 32);
    int16_t temporary{pitch};
    if(pitch < resting_pitch) {
      temporary = high(response.pitch);
      if(temporary >= 0) {
        response.pitch = maths::dword(-static_cast<int64_t>(add(response.pitch, 1000))) >> 1;
        temporary = maths::word(response.pitch);
      }
    }
    if(temporary >= 0 && high(response.pitch) < 0) response.pitch = maths::dword(-static_cast<int64_t>(add(response.pitch, -1000))) >> 1;
  } else if(aircraft.stalled) response.pitch = add(response.pitch, static_cast<int32_t>(aircraft.body_velocity.z) * 128);

  bool protect{false};
  if(!aircraft.on_ground) {
    uint16_t const stall_angle{static_cast<uint16_t>(aircraft.flaps ? 3882 : 2912)};
    unsigned int const reduced{(static_cast<unsigned int>(stall_angle) * 58'000) >> 16};
    response.almost_stall_angle = static_cast<uint16_t>(static_cast<unsigned int>(aircraft.density) * reduced / environment.sea_level_density);
  }
  // on the ground the native branch tests the ground flag instead of the stalled word
  if(!aircraft.on_ground && !aircraft.stalled) {
    int16_t const forecast{maths::word(aircraft.angle_of_attack * 2 - aircraft.previous_angle_of_attack)};
    uint16_t const magnitude{static_cast<uint16_t>(forecast < 0 ? maths::negate(forecast) : forecast)};
    if(response.stall_protection) protect = maths::word(std::abs(static_cast<int>(high(response.pitch))) - response.last_protected_pitch_rate) >= 0;
    else protect = magnitude >= response.almost_stall_angle;
    if(protect && magnitude != 0) {
      response.pitch = maths::scale(maths::rounded_divide(response.pitch, maths::word(magnitude)), maths::word(response.almost_stall_angle));
      response.last_protected_pitch_rate = static_cast<uint16_t>(std::abs(static_cast<int>(high(response.pitch))));
    }
  }
  response.stall_protection = protect;

  response.roll = replace_high(response.roll, std::clamp<int16_t>(high(response.roll), -maximum_rate, maximum_rate));
  response.pitch = replace_high(response.pitch, std::clamp<int16_t>(high(response.pitch), -maximum_rate, maximum_rate));
  response.yaw = replace_high(response.yaw, std::clamp<int16_t>(high(response.yaw), -maximum_rate, maximum_rate));
  maths::angles const change{
    .heading{static_cast<maths::angle>(static_cast<int32_t>(high(response.yaw)) * step.elapsed / step.normal)},
    .pitch{static_cast<maths::angle>(static_cast<int32_t>(maths::negate(high(response.pitch))) * step.elapsed / step.normal)},
    .roll{static_cast<maths::angle>(static_cast<int32_t>(high(response.roll)) * step.elapsed / step.normal)},
  };
  aircraft.attitude = maths::extract_angles(maths::rotate_local(maths::make_inverse_rotation(aircraft.attitude), change));
}

} // namespace corncob::flight
