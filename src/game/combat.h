#pragma once

#include <optional>
#include "flight/aircraft.h"
#include "game/statistics.h"
#include "maths/random.h"
#include "resources/geometry_bank.h"
#include "world/world.h"

namespace corncob::game {

struct weapon_controls {
  bool gun{false};
  bool cannon{false};
  bool missile{false};
};
struct weapon_origin {
  maths::world_position const &position;
  maths::angles const &attitude;
  maths::rotation const &body_to_world;
  int16_t forward_airflow;
  bool on_foot;
  bool remote;
  bool crashed;
};
enum class fragment_pattern { outward, upward, mortar, incendiary };

struct sound_cues {
  uint32_t gunshots{0};
  uint32_t missiles{0};
  uint32_t explosions{0};
};

struct combat_statistics : sortie_statistics {
  unsigned int resources_destroyed{0};
  unsigned int orbs_destroyed{0};

};

class combat {
private:
  resources::geometry_bank const &geometry;
  world::theatre &theatre;
  maths::random_source random;
  flight::time_step tick_step;
  unsigned int scenery_count;
  unsigned int effect_cursor{0};
  unsigned int bullet_cursor{0};
  unsigned int gun_average{0};
  unsigned int missile_average{0};
  uint32_t gun_ready_tick{0};
  uint32_t missile_ready_tick{0};
  uint32_t bomb_replacement_tick{0};
  unsigned int bombs{3};
  unsigned int pistol_rounds{20};
  bool charge_available{true};
  int16_t muzzle_offset{720};
  int16_t missile_offset{240};
  combat_statistics statistics;
  sound_cues cues;

  std::optional<unsigned int> allocate(bool bullet = false);
  void detonate(unsigned int index);
  void collide(unsigned int a, unsigned int b);
  void update_projectile(unsigned int index, maths::world_position const &observer, flight::time_step step);

public:
  static unsigned int constexpr effect_capacity{320};
  static unsigned int constexpr bullet_capacity{32};
  static unsigned int constexpr bomb_capacity{3};

  combat(resources::geometry_bank const &geometry, world::theatre &theatre, uint32_t seed = 2'949'243u);
  void scatter(world::placed_object const &source, unsigned int count, fragment_pattern pattern, flight::time_step step);
  void rearm_person() noexcept;
  std::optional<unsigned int> spawn(world::object_type type, maths::world_position const &position);
  void explosion(maths::world_position const &position, uint16_t damaging_flags = 0);
  bool shoot(weapon_origin const &origin, bool cannon, flight::time_step step);
  bool launch_missile(weapon_origin const &origin, flight::time_step step);
  bool drop_bomb(flight::aircraft_state &aircraft, uint32_t tick, flight::time_step step);
  bool plant_charge(maths::world_position const &position);
  void arm_charges();
  void detonate_object(unsigned int index);
  void fire(weapon_controls const &controls, weapon_origin const &origin, uint32_t tick, flight::time_step step);
  void move(maths::world_position const &observer, flight::time_step step);
  void collisions(maths::world_position const &observer);
  void advance(maths::world_position const &observer, flight::time_step step);
  void damage(unsigned int index, world::placed_object const &attacker, bool aircraft_impact = false);
  void complete(world::placed_object &object);
  void rearm() noexcept;
  combat_statistics const &stats() const noexcept;
  sound_cues const &sounds() const noexcept;
  unsigned int bomb_supply() const noexcept;
  unsigned int ammunition() const noexcept;
  unsigned int stored_objects() const noexcept;
  void observe_defences(quality_counts const &aaa, quality_counts const &kla);
};

bool objects_overlap(world::placed_object const &a, world::placed_object const &b) noexcept;

} // namespace corncob::game
