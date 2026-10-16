#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include "game/joystick.h"
#include "tests/reference_reader.h"

int main(int const argc, char const *const argv[]) {
  /// Match the original calibrated response and check that physical flight controls remain independent of keyboard head views
  if(argc != 2) throw std::invalid_argument{"joystick response fixture required"};
  namespace game = corncob::game;
  corncob::tests::reference_reader fixture{argv[1], "CORNJOY1"};
  auto const count{fixture.dword()};
  for(unsigned int i{0}; i != count; ++i) {
    auto const sample{fixture.signed_word()}, expected{fixture.signed_word()};
    if(game::joystick_axis(sample) != expected) throw std::runtime_error{"native joystick response case " + std::to_string(i)};
  }
  game::joystick_sample const joystick{
    .axes{32'767, -32'767},
    .gun{true},
    .missile{true},
  };
  game::input_state keyboard{
    .stick{
      .roll{256},
    },
  };
  game::apply_joystick(keyboard, joystick, false);
  if(keyboard.stick.roll != 256 || keyboard.stick.pitch != 0 || keyboard.joystick_controlled || !keyboard.weapons.gun || !keyboard.weapons.missile)
    throw std::runtime_error{"keyboard priority changed independent joystick buttons"};
  game::apply_joystick(keyboard, joystick, true);
  if(keyboard.stick.roll != 512 || keyboard.stick.pitch != -512 || !keyboard.joystick_controlled)
    throw std::runtime_error{"head-view keys suppressed physical flight control"};
  game::input_state space{
    .weapons{
      .gun{true},
    },
  };
  game::apply_joystick(space, {}, false);
  if(space.weapons.gun || !space.weapons.missile) throw std::runtime_error{"original Space missile request was lost in joystick mode"};
  std::cout << count << " native joystick responses, keyboard priority, head views and button rules match" << std::endl;
  return EXIT_SUCCESS;
}
