#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include "flight/damage.h"
#include "tests/reference_reader.h"

int main(int const argc, char const *const argv[]) {
  /// Compare randomized damage channels and the exact PRNG continuation with native DAMAGEPLANE
  if(argc != 2) throw std::invalid_argument{"damage_checks requires a fixture path"};
  corncob::tests::reference_reader input{argv[1], "CORNDAM1"};
  auto const cases{input.dword()};
  for(unsigned int i{0}; i != cases; ++i) {
    corncob::flight::damage_state damage{
      .total{input.word()},
      .worst{input.word()},
      .wing{input.signed_word()},
      .rudder{input.signed_word()},
      .stability{input.signed_word()},
      .wing_loss{input.signed_word()},
      .engine{input.word()},
      .maximum_throttle{input.word()},
    };
    corncob::flight::aircraft_state aircraft;
    aircraft.throttle = input.word();
    auto const events{input.word()};
    corncob::flight::time_step const step{
      .elapsed{input.signed_word()},
      .normal{5},
    };
    damage.invincible = input.word() != 0;
    corncob::maths::random_source random;
    corncob::flight::apply_damage(damage, aircraft, events, random, step);
    std::array<uint16_t, 9> const actual{damage.total, damage.worst, static_cast<uint16_t>(damage.wing), static_cast<uint16_t>(damage.rudder),
      static_cast<uint16_t>(damage.stability), static_cast<uint16_t>(damage.wing_loss), damage.engine, damage.maximum_throttle, aircraft.throttle};
    for(unsigned int field{0}; field != actual.size(); ++field) {
      auto const expected{input.word()};
      if(actual[field] != expected) throw std::runtime_error{"native damage case " + std::to_string(i) + " field " + std::to_string(field)};
    }
    if(random.seed() != input.dword()) throw std::runtime_error{"native damage PRNG continuation differs"};
  }
  std::cout << cases << " native randomized damage passes match" << std::endl;
  return EXIT_SUCCESS;
}
