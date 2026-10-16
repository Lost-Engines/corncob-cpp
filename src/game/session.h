#pragma once

#include <optional>
#include <string>
#include "flight/landing.h"
#include "game/map_view.h"
#include "flight/damage.h"
#include "game/combat.h"
#include "game/actors.h"
#include "graphics/scene.h"
#include "resources/geometry_bank.h"
#include "resources/radio_messages.h"
#include "world/world.h"

namespace corncob::game {

enum class player_mode {
  on_foot,
  in_aircraft,
  dead,
  captured,
  returned,
  rescued,
  earth_saved,
};

enum class action {
  throttle_up,
  throttle_down,
  toggle_flaps,
  toggle_autopilot,
  rocket,
  eject,
  parachute,
  toggle_remote,
  toggle_rear,
  toggle_cockpit,
  toggle_ground,
  toggle_look,
  toggle_map,
  call_rescue,
  toggle_messages,
  pause,
  drop_bomb,
  plant_charge,
  arm_charges,
};

struct view_input {
  bool left{false};
  bool right{false};
  bool up{false};
  bool down{false};
  bool below{false};
  bool modifier{false};
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
  weapon_controls weapons{};
  view_input view{};
  map_input map{};
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
  combat weapons;
  actors enemies;
  flight::aircraft_state aircraft;
  flight::damage_state damage;
  maths::random_source random;
  uint32_t last_wall_damage{0};
  uint16_t sortie_flags{0};
  uint16_t aircraft_losses{0};
  world::object_type fatal_contact{world::object_type::count};
  bool airframe_destroyed{false};
  bool left_ground{false};
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
  bool look_mode{false};
  bool looking{false};
  maths::angles head_angles;
  std::optional<map_view> map;
  bool experimental_clearance{false};
  bool invincible{false};
  uint16_t orb_distortion{0};
  vec3<uint16_t> palette_phase{0, 0, 0};
  uint32_t fast_ticks{0};
  unsigned int frame{0};
  std::string notice;
  bool messages{true};
  bool low_altitude{false};
  bool steering_warned{false};
  std::optional<resources::radio_notice> radio;

  void update_aircraft(input_state const &input, flight::time_step step);
  void update_scenery();
  void update_person(input_state const &input, flight::time_step step);
  void find_surfaces();
  void check_aircraft_entry();
  void check_aircraft_collisions(flight::time_step step);
  void enter_portal(world::placed_object &portal);
  bool at_home() const;
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
  bool autopilot_engaged() const noexcept;
  bool looking_around() const noexcept;
  bool in_map() const noexcept;
  bool messages_enabled() const noexcept;
  std::optional<resources::radio_notice> take_radio();
  flight::damage_state const &aircraft_damage() const noexcept;
  combat_statistics const &statistics() const noexcept;
  sound_cues const &sounds() const noexcept;
  bool remote_control() const noexcept;
  void relocate(world::mission_start const &start);
  void set_invincible(bool value) noexcept;
  void set_experimental_clearance(bool permitted) noexcept;
  sortie_statistics results(bool aborted = false) const;
  world::theatre snapshot() const;
  bool finished() const noexcept;
};

} // namespace corncob::game
