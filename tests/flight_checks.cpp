#include <array>
#include <bit>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include "flight/aircraft.h"

namespace {

class fixture {
private:
  std::ifstream input;

public:
  fixture(char const *const path) :
    input{path, std::ios::binary} {
    /// Fail on truncated data rather than treating EOF as a native arithmetic result
    input.exceptions(std::ios::failbit | std::ios::badbit);
    std::array<char, 8> magic;
    input.read(magic.data(), magic.size());
    if(std::string_view{magic.data(), magic.size()} != "CORNFY01") throw std::runtime_error{"invalid native flight fixture"};
  }
  uint8_t byte() {
    /// Read one native byte without relying on char signedness
    return static_cast<uint8_t>(input.get());
  }
  uint16_t word() {
    /// Decode the fixture's explicit little-endian word order
    unsigned int const low{byte()}, high{byte()};
    return static_cast<uint16_t>(low | (high << 8));
  }
  int16_t signed_word() {
    /// Preserve the native sign bit
    return std::bit_cast<int16_t>(word());
  }
  uint32_t dword() {
    /// Decode a count or dword without host structure padding
    uint32_t const low{word()};
    return low | (static_cast<uint32_t>(word()) << 16);
  }
  int32_t signed_dword() {
    /// Read signed world positions and force accumulators
    return std::bit_cast<int32_t>(dword());
  }
};

template<typename value_type>
void check(value_type const actual, value_type const expected, char const *const name, unsigned int const index) {
  /// Give the first differing original intermediate, not only a final flight trajectory mismatch
  if(actual != expected) throw std::runtime_error{std::string{name} + " case " + std::to_string(index)
    + ": expected " + std::to_string(expected) + ", got " + std::to_string(actual)};
}

corncob::flight::aircraft_variant variant(uint16_t const flags) {
  /// Decode the two aircraft flags used by native force and integration probes
  if((flags & 0x2000u) != 0) return corncob::flight::aircraft_variant::experimental;
  if((flags & 0x1000u) != 0) return corncob::flight::aircraft_variant::reserve;
  return corncob::flight::aircraft_variant::standard;
}

} // anonymous namespace

int main(int const argc, char const *const argv[]) {
  /// Compare the original atmosphere, force and integration code with its native C++ translation
  if(argc != 2) throw std::invalid_argument{"flight_checks requires its reference fixture path"};
  fixture input{argv[1]};
  unsigned int const angles{input.dword()}, forces{input.dword()}, positions{input.dword()};
  for(unsigned int i{0}; i != angles; ++i) {
    auto const x{input.signed_word()}, y{input.signed_word()};
    check(corncob::maths::arctangent(x, y), input.word(), "airflow angle", i);
  }
  for(unsigned int i{0}; i != forces; ++i) {
    corncob::flight::aircraft_state aircraft;
    corncob::maths::angles const attitude{
      .heading{input.word()},
      .pitch{input.word()},
      .roll{input.word()},
    };
    aircraft.world_to_body = corncob::maths::make_rotation(attitude);
    aircraft.body_to_world = corncob::maths::make_inverse_rotation(attitude);
    if(i < 8) {
      // the eight established research probes use literal identity matrices
      for(unsigned int row{0}; row != 3; ++row) {
        for(unsigned int column{0}; column != 3; ++column) {
          aircraft.world_to_body.rows[row][column] = row == column ? 32'767 : 0;
          aircraft.body_to_world.rows[row][column] = row == column ? 32'767 : 0;
        }
      }
    }
    for(unsigned int axis{0}; axis != 3; ++axis) aircraft.velocity[axis] = input.signed_word();
    aircraft.throttle = input.word();
    aircraft.flaps = input.word() != 0;
    aircraft.position.z = input.signed_dword();
    corncob::flight::atmosphere environment{
      .gravity{0},
      .sea_level_density{input.word()},
      .height{input.word()},
    };
    environment.gravity = input.signed_word();
    corncob::flight::time_step const step{
      .elapsed{input.signed_word()},
      .normal{5},
    };
    aircraft.variant = variant(input.word());
    aircraft.on_ground = input.word() != 0;
    aircraft.rocket = input.word() != 0;
    aircraft.rocket_ticks = input.signed_word();
    aircraft.density = input.word();
    aircraft.angle_of_attack = input.signed_word();
    corncob::flight::advance_forces(aircraft, environment, step);
    for(unsigned int axis{0}; axis != 3; ++axis) check(aircraft.body_force[axis], input.signed_dword(), "body force", i);
    for(auto const *const vector : {&aircraft.body_velocity, &aircraft.scaled_body_velocity, &aircraft.body_acceleration,
                                   &aircraft.acceleration, &aircraft.velocity}) {
      for(unsigned int axis{0}; axis != 3; ++axis) check((*vector)[axis], input.signed_word(), "force velocity/acceleration", i);
    }
    check(aircraft.density, input.word(), "density", i);
    check(aircraft.power_factor, input.word(), "power factor", i);
    check(aircraft.angle_of_attack, input.signed_word(), "angle of attack", i);
    check(aircraft.previous_angle_of_attack, input.signed_word(), "previous angle of attack", i);
    check(aircraft.stalled, input.word() != 0, "stalled", i);
    check(aircraft.side_force, input.signed_word(), "yaw side force", i);
    check(aircraft.rocket, input.byte() != 0, "rocket enabled", i);
    check(aircraft.rocket_ticks, input.signed_word(), "rocket timer", i);
  }
  for(unsigned int i{0}; i != positions; ++i) {
    corncob::flight::aircraft_state aircraft;
    for(unsigned int axis{0}; axis != 3; ++axis) aircraft.position[axis] = input.signed_dword();
    for(unsigned int axis{0}; axis != 3; ++axis) aircraft.velocity[axis] = input.signed_word();
    for(unsigned int axis{0}; axis != 3; ++axis) aircraft.acceleration[axis] = input.signed_word();
    aircraft.variant = variant(input.word());
    corncob::flight::time_step const step{
      .elapsed{input.signed_word()},
      .normal{5},
    };
    corncob::flight::advance_position(aircraft, step);
    for(unsigned int axis{0}; axis != 3; ++axis) check(aircraft.position[axis], input.signed_dword(), "position integration", i);
  }
  std::cout << angles << " native airflow angles, " << forces << " force passes and " << positions << " position steps match" << std::endl;
  return EXIT_SUCCESS;
}
