#pragma once

#include <optional>
#include <string>
#include "flight/landing.h"
#include "graphics/scene.h"
#include "resources/geometry_bank.h"
#include "world/world.h"

namespace corncob::game {

enum class player_mode {
  on_foot,
  in_aircraft,
  dead,
  captured,
};

enum class action {
  throttle_up,
  throttle_down,
  toggle_flaps,
  rocket,
  eject,
  parachute,
  toggle_remote,
  toggle_rear,
  toggle_cockpit,
  toggle_ground,
  pause,
};

struct input_state {
  flight::control_input stick{};
  bool throttle_up{false};
  bool throttle_down{false};
  bool walk_forward{false};
  bool walk_backward{false};
  bool jump{false};
  bool drop_through{false};
  bool advance_time{false};
};

struct person_state {
  maths::world_position position{0, 0, 720};
  maths::angles attitude;
  maths::fixed_vector velocity{0, 0, 0};
  int32_t surface_height{0};
  uint16_t jump_charge{0};
  uint16_t last_step_tick{0};
  int16_t step_wobble{80};
  int16_t horizontal_damping{32'000};
  bool standing{true};
  bool parachute{false};
  bool charging_jump{false};
};

class session {
private:
  resources::geometry_bank const &geometry;
  world::theatre theatre;
  flight::aircraft_state aircraft;
  flight::angular_response angular;
  flight::landing_state landing;
  flight::atmosphere atmosphere;
  person_state person;
  std::optional<unsigned int> controlled_aircraft;
  player_mode mode{player_mode::on_foot};
  bool reentry_allowed{false};
  bool remote{false};
  bool rear{true};
  bool cockpit{true};
  bool ground_detail{true};
  bool paused{false};
  bool experimental_clearance{false};
  uint32_t fast_ticks{0};
  unsigned int frame{0};
  std::string notice;

  void update_aircraft(input_state const &input, flight::time_step step);
  void update_scenery();
  void update_person(input_state const &input, flight::time_step step);
  void find_surfaces();
  void check_aircraft_entry();
  void enter_aircraft(unsigned int index);
  void leave_aircraft();
  void retain_aircraft();

public:
  session(resources::geometry_bank const &geometry, world::theatre theatre);
  void press(action command);
  void advance(input_state const &input);
  graphics::camera camera() const;
  world::theatre const &world() const noexcept;
  player_mode player() const noexcept;
  flight::aircraft_state const &craft() const noexcept;
  person_state const &pilot() const noexcept;
  std::string const &message() const noexcept;
  uint32_t timer() const noexcept;
  unsigned int frame_number() const noexcept;
  bool is_paused() const noexcept;
  void set_experimental_clearance(bool permitted) noexcept;
};

} // namespace corncob::game
