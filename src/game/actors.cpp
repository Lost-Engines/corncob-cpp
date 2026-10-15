#include "actors.h"
#include <algorithm>
#include <bit>
#include <cstdlib>
#include "maths/integer.h"
#include "resources/actor_tables.h"
#include "world/motion.h"
#include "vectorstorm/vector/vector2.h"

namespace corncob::game {

namespace {

struct target_vector {
  maths::fixed_vector direction{0, 0, 0};
  uint16_t distance;
  maths::angles bearing;
};
std::optional<target_vector> relative_target(maths::world_position const &object, maths::world_position const &target, unsigned int const shift = 6) {
  /// GETOBSVEC and RADARPROCESS use different word scales but the same one-megacentimetre component gate
  target_vector result{};
  for(unsigned int axis{0}; axis != 3; ++axis) {
    auto const delta{maths::dword(static_cast<int64_t>(object[axis]) - target[axis])};
    if(std::abs(static_cast<int64_t>(delta)) >= 1'048'576) return std::nullopt;
    result.direction[axis] = maths::negate(maths::word(delta >> shift));
  }
  maths::fixed_vector const horizontal{result.direction.x, result.direction.y, 0};
  uint16_t const horizontal_distance{maths::magnitude(horizontal)};
  result.distance = shift == 4 ? horizontal_distance : maths::magnitude(result.direction);
  result.bearing = {
    .heading{static_cast<maths::angle>(maths::arctangent(maths::negate(result.direction.x), maths::negate(result.direction.y)) + 32'768)},
    .pitch{maths::arctangent(maths::word(horizontal_distance), maths::negate(result.direction.z))},
  };
  return result;
}
int16_t timed(int16_t const value, flight::time_step const step) noexcept {
  /// WTADJ uses signed truncation for turn limits and small per-tick adjustments
  return maths::word(static_cast<int32_t>(value) * step.elapsed / step.normal);
}
int16_t steer(maths::angle const desired, maths::angle &current, uint16_t const limit, flight::time_step const step) noexcept {
  /// Ground guns and KLAs ease one sixteenth of the wrapped angular error, bounded by quality
  int16_t const error{static_cast<int16_t>(maths::word(desired - current) >> 4)};
  current += static_cast<maths::angle>(timed(static_cast<int16_t>(std::clamp<int>(error, -limit, limit)), step));
  return error;
}
bool objective_in_range(world::placed_object const &object, resources::object_definition const &definition, maths::world_position const &observer) {
  /// HQTIM always checks near-tier anchors and otherwise compares the tier-scaled squared distance with the middle limit
  auto const delta{vec3<int64_t>{object.position} - vec3<int64_t>{observer}};
  int64_t const maximum{std::max({std::abs(delta.x), std::abs(delta.y), std::abs(delta.z)})};
  if(maximum < 32'768) return true;
  unsigned int const shift{maximum < 524'288 ? 4u : 8u};
  int64_t distance{0};
  for(unsigned int axis{0}; axis != 3; ++axis) {
    int64_t const component{delta[axis] >> shift};
    distance += component * component;
  }
  return distance <= definition.limits.middle_squared;
}

} // anonymous namespace

actors::actors(resources::geometry_bank const &geometry, world::theatre &theatre, combat &weapons) :
  geometry{geometry},
  theatre{theatre},
  weapons{weapons} {
  /// Keep the preceding active census separate from this pass's live actor updates
}
void actors::update_launcher(world::placed_object &object, actor_context const &context, flight::time_step const step) {
  /// Launcher countdowns and local limits determine spawning; qualities pass through to the spawned actor
  using enum world::object_type;
  auto const population{[&](world::object_type const type){return previous_counts[std::to_underlying(type)];}};
  unsigned int const quality{object.quality()};
  unsigned int reset{400};
  world::object_type launched{kla};
  unsigned int available{population(kla_site)}, present{population(kla)};
  if(object.type == vehicle_site) {
    reset = resources::vehicle_site_qualities[quality].launch_reset;
    launched = ground_vehicle;
    available = std::max(population(vehicle_site), 4u);
    present = population(ground_vehicle);
  } else if(object.type == saucer_site) {
    reset = resources::saucer_site_qualities[quality].launch_reset;
    launched = saucer;
    available = population(saucer_site);
    present = population(saucer) + population(objective_saucer);
  }
  unsigned int const trigger{object.type == vehicle_site ? reset : 100u};
  if(object.timer >= trigger || present >= available) return;
  if(object.type == saucer_site && random.next() > static_cast<uint16_t>(timed(100, step))) return;
  object.timer = static_cast<uint16_t>(reset);
  auto const index{weapons.spawn(launched, object.position)};
  if(!index) return;
  auto &spawned{theatre.objects[*index]};
  spawned.flags = static_cast<uint16_t>((spawned.flags & ~0x7000u) | (quality << 12));
  if(launched == kla) {
    spawned.timer = 24'000;
    spawned.attitude.pitch = static_cast<maths::angle>(-16'384);
    spawned.velocity = maths::fixed_vector{0, 0, 0};
  } else if(launched == ground_vehicle) {
    spawned.timer = static_cast<uint16_t>(10'000 * step.normal / step.elapsed);
    spawned.velocity = maths::fixed_vector{0, 0, 0};
    spawned.collision_group = static_cast<uint8_t>(-static_cast<int>(present + 2));
    object.collision_group = spawned.collision_group;
  } else {
    spawned.timer = 24'000;
    spawned.position.z += 5000;
  }
  (void)context;
}
void actors::update_kla(world::placed_object &object, actor_context const &context, flight::time_step const step) {
  /// KLAs launch vertically, wait through their hundred-frame rest and then steer rather than teleporting their velocity
  if(object.timer >= 23'900) return;
  object.motion.gravity = static_cast<int16_t>(static_cast<uint16_t>(object.motion.gravity) & 0xff00u);
  object.motion.dispersion = 0;
  object.motion.drag_divisor = 0;
  auto const target{relative_target(object.position, context.target, 4)};
  if(!target) {
    object.velocity = maths::fixed_vector{0, 0, 0};
    if(object.timer <= 11'950) object.timer = 1;
    return;
  }
  auto const &parameters{resources::kla_qualities[object.quality()]};
  int16_t const pitch_error{steer(target->bearing.pitch, object.attitude.pitch, parameters.pitch_turn_limit, step)};
  int16_t const heading_error{steer(target->bearing.heading, object.attitude.heading, parameters.heading_turn_limit, step)};
  object.attitude.roll += static_cast<maths::angle>(timed(maths::word(parameters.roll_rate), step));
  unsigned int const error{static_cast<unsigned int>(std::max(std::abs(static_cast<int>(pitch_error)), std::abs(static_cast<int>(heading_error))))};
  int target_speed{parameters.normal_target_speed};
  if(error <= parameters.lock_angle_limit && target->distance >= 200) target_speed = parameters.locked_target_speed;
  else if(error > parameters.lock_angle_limit) target_speed = parameters.off_axis_target_speed;
  int speed{maths::magnitude(object.velocity)};
  speed += (target_speed - speed) / parameters.speed_relaxation_divisor
    + random.below(parameters.speed_noise_span) - parameters.speed_noise_span / 2;
  object.velocity = maths::transform(maths::make_inverse_rotation(object.attitude), {maths::word(speed), 0, 0});
  if(object.position.z >= 0 && object.position.z <= 1000 && object.velocity.z < 0) object.velocity.z = 0;
}
void actors::update_aaa(world::placed_object &object, actor_context const &context, flight::time_step const step) {
  /// AAA quality controls tracking, target lead, firing probability and in-flight shell jitter
  auto target{relative_target(object.position, context.target)};
  if(!target) {
    object.velocity = maths::fixed_vector{0, 0, 0};
    return;
  }
  auto const &parameters{resources::aaa_qualities[object.quality()]};
  int16_t constexpr shell_speed{1500};
  if(parameters.lead_target != 0) {
    for(unsigned int axis{0}; axis != 3; ++axis) target->direction[axis] = maths::word(target->direction[axis]
      + static_cast<int32_t>(maths::word(target->distance)) * (context.target_velocity[axis] >> 4) / shell_speed);
    maths::fixed_vector const horizontal{target->direction.x, target->direction.y, 0};
    target->bearing.heading = static_cast<maths::angle>(maths::arctangent(maths::negate(target->direction.x), maths::negate(target->direction.y)) + 32'768);
    target->bearing.pitch = maths::arctangent(maths::word(maths::magnitude(horizontal)), maths::negate(target->direction.z));
  }
  steer(target->bearing.pitch, object.attitude.pitch, parameters.pitch_turn_limit, step);
  steer(target->bearing.heading, object.attitude.heading, parameters.heading_turn_limit, step);
  if(object.position.z >= 0 && object.position.z <= 2000 && maths::word(object.attitude.pitch) >= -1200)
    object.attitude.pitch = static_cast<maths::angle>(-1200);
  if(random.next() >= parameters.fire_threshold) return;
  auto const index{weapons.spawn(world::object_type::aa_shell, object.position)};
  if(!index) return;
  auto &shell{theatre.objects[*index]};
  shell.position.z += 1600;
  shell.velocity = maths::transform(maths::make_inverse_rotation(object.attitude), {shell_speed, 0, 0});
  shell.timer = static_cast<uint16_t>(150 * step.normal / step.elapsed);
  shell.motion.dispersion = maths::word(parameters.shell_jitter);
}
void actors::update_saucer(world::placed_object &object, actor_context const &context, flight::time_step const step) {
  /// Saucers cycle attack, retreat, sideways and coasting modes around the current airbase rather than chasing the pilot directly
  auto const &parameters{resources::saucer_qualities[object.quality()]};
  if(!object.has(world::object_flag::collision_callback)) {
    object.motion.gravity = 15;
    object.motion.dispersion = 20;
    object.motion.drag_divisor = 18'000;
    if(object.position.z < 1000) {
      weapons.explosion(object.position);
      object.flags = 0;
    }
    return;
  }
  if(object.type == world::object_type::objective_saucer && object.timer >= 32'700) {
    if((object.flags & 0x4000u) == 0) object.timer = 32'000;
    else if(object.health >= 25) {
      object.timer = 32'767;
      return;
    } else {
      object.health = 50;
      object.timer = 32'000;
    }
  }
  object.timer = static_cast<uint16_t>(std::max(0, static_cast<int>(object.timer) + 1 - timed(3, step)));
  object.attitude.roll = static_cast<maths::angle>(object.attitude.roll - (maths::word(object.attitude.roll) >> 4));
  object.attitude.heading += 4000;
  int16_t const height_error{maths::word((object.position.z - 3 * 65'536) >> 8)};
  int16_t rise{maths::word(-height_error / 100)};
  if(std::abs(static_cast<int>(rise)) >= 40) rise = std::clamp<int16_t>(rise, -15, 15);
  object.velocity.z = maths::word(object.velocity.z + timed(rise, step));
  int const damping{std::max(1, 20 * step.normal / step.elapsed)};
  object.velocity.z = maths::word(object.velocity.z - object.velocity.z / damping);
  vec2<int32_t> direction{theatre.start.x - object.position.x, theatre.start.y - object.position.y};
  unsigned int shifts{0};
  while((std::abs(static_cast<int64_t>(direction.x)) > 10 || std::abs(static_cast<int64_t>(direction.y)) > 10) && shifts != 31) {
    direction.x >>= 1;
    direction.y >>= 1;
    ++shifts;
  }
  uint16_t transition{3};
  switch(object.flags & 6u) {
  case 0:
    object.velocity.x = maths::word(object.velocity.x + direction.x * 2);
    object.velocity.y = maths::word(object.velocity.y + direction.y * 2);
    transition = shifts <= 13 ? 50 : object.health <= 25 ? 20 : 3;
    break;
  case 2:
    object.velocity.x = maths::word(object.velocity.x - direction.x);
    object.velocity.y = maths::word(object.velocity.y - direction.y);
    transition = parameters.retreat_exit_threshold;
    break;
  case 4:
    object.velocity.y = maths::word(object.velocity.y + direction.x);
    object.velocity.x = maths::word(object.velocity.x - direction.y);
    transition = parameters.sideways_exit_threshold;
    break;
  case 6:
    transition = parameters.coast_exit_threshold;
    break;
  }
  if((random.next() >> 1) <= static_cast<uint16_t>(timed(maths::word(transition), step)))
    object.flags = static_cast<uint16_t>((object.flags & ~6u) | ((object.flags + 2u) & 6u));
  if(shifts <= 13) {
    if(object.position.z >= 65'536 && (random.next() >> 1) <= parameters.bomb_threshold) {
      if(auto const index{weapons.spawn(world::object_type::bomb, object.position)}) {
        auto &bomb{theatre.objects[*index]};
        bomb.position.z -= 500;
        bomb.velocity = maths::fixed_vector{object.velocity};
        bomb.flags = static_cast<uint16_t>((bomb.flags | 2u) & ~0xe8u);
        bomb.timer = 3200;
      }
    }
  } else if((random.next() >> 1) <= parameters.kla_threshold
    && previous_counts[std::to_underlying(world::object_type::kla)] <= (parameters.weapon_bits & 255u)) {
    if(auto const index{weapons.spawn(world::object_type::kla, object.position)}) {
      auto &kla{theatre.objects[*index]};
      kla.flags = static_cast<uint16_t>((kla.flags & ~0x7000u) | (object.flags & 0x7000u));
      kla.motion.gravity = 20;
      kla.motion.dispersion = 20;
      kla.motion.drag_divisor = 16'000;
      kla.timer = static_cast<uint16_t>(23'900 + 60 * step.normal / step.elapsed);
    }
  }
  unsigned int phase{static_cast<unsigned int>(32'000 - object.timer)};
  if(phase >= 1000) {
    object.timer = 32'000;
    phase = 0;
  }
  phase = phase * static_cast<unsigned int>(step.elapsed) / static_cast<unsigned int>(step.normal);
  if(parameters.vertical_wave_amplitude != 0 && object.position.z >= 65'536) {
    auto const wave{maths::negate(maths::sine(static_cast<maths::angle>(phase * parameters.vertical_wave_frequency * 8)))};
    object.velocity.z = maths::word(object.velocity.z + maths::multiply(wave, maths::word(parameters.vertical_wave_amplitude)));
  }
  if(parameters.horizontal_wave_amplitude != 0) {
    auto const angle{static_cast<maths::angle>(phase * parameters.horizontal_wave_frequency * 8)};
    object.velocity.x = maths::word(object.velocity.x + maths::multiply(maths::negate(maths::sine(angle)), maths::word(parameters.horizontal_wave_amplitude)));
    object.velocity.y = maths::word(object.velocity.y + maths::multiply(maths::negate(maths::cosine(angle)), 100));
  }
  int const speed{maths::magnitude(object.velocity)};
  int const desired{speed + (static_cast<int>(parameters.target_speed) - speed) / parameters.horizontal_wave_frequency
    + random.below(parameters.speed_noise_span) - parameters.speed_noise_span / 2};
  // the original only reduces this magnitude; acceleration comes from its mode and wave terms
  if(desired <= speed && speed != 0) {
    for(unsigned int axis{0}; axis != 3; ++axis) object.velocity[axis] = maths::word(static_cast<int32_t>(object.velocity[axis]) * desired / speed);
  }
  if((parameters.weapon_bits >> 8) != 0) {
    world::placed_object gun{
      .position{maths::world_position{object.position}},
      .attitude{},
      .type{world::object_type::aaa_site},
      .flags{static_cast<uint16_t>((object.flags & ~0x7000u) | (std::min(object.quality(), 5u) << 12))},
    };
    update_aaa(gun, context, step);
  }
}
void actors::update_shell(unsigned int const index, actor_context const &context, flight::time_step const step) {
  /// AAA shells burst on their last frame or probabilistically near aircraft altitude, expanding their contact volume
  auto &object{theatre.objects[index]};
  if(object.timer == 1) {
    weapons.detonate_object(index);
    return;
  }
  if(object.position.z >= 0 && object.position.z <= 65'535 && (static_cast<uint32_t>(object.position.z) >> 1) <= static_cast<uint16_t>(object.collision_extents.x)) return;
  auto const delta{maths::dword(static_cast<int64_t>(object.position.z) - context.target.z)};
  if(std::abs(static_cast<int64_t>(delta)) <= 4500 && random.next() <= static_cast<uint16_t>(timed(3000, step))) {
    weapons.detonate_object(index);
    return;
  }
  object.collision_extents = maths::fixed_vector{6000, 6000, 6000};
}
void actors::update_vehicle(world::placed_object &object, actor_context const &context, flight::time_step const step) {
  /// Ground vehicles alternate wandering and pursuit, then slow and halt around an exposed pilot
  if((object.flags & 255u) == 0x71u) return;
  object.position.z = 0;
  auto const &parameters{resources::vehicle_qualities[object.quality()]};
  auto const target{relative_target(object.position, context.observer)};
  bool const near{target && target->distance < 512};
  unsigned int threshold{static_cast<unsigned int>(timed(maths::word(parameters.change_direction_threshold), step))};
  if(near && (object.flags & 4u) == 0) threshold *= 4;
  if((object.flags & 4u) != 0 || random.next() <= threshold) {
    if((object.flags & 4u) == 0) {
      object.attitude.heading = random.next();
      object.attitude.pitch = object.attitude.roll = 0;
    }
    unsigned int const pursuit{near ? parameters.near_pursuit_threshold : parameters.far_pursuit_threshold};
    if(target && (near || random.next() <= pursuit)) {
      if((object.flags & 4u) == 0) object.attitude.heading = target->bearing.heading;
      if(context.on_foot && target->distance <= 256) {
        object.flags |= 2u;
        effects.capture_pending = true;
        if(target->distance <= 100) {
          object.flags |= 4u;
          effects.captured = true;
        }
        if(object.collision_group == 1) {
          effects.captured = false;
          effects.capture_pending = false;
          if(target->distance <= 32) effects.rescued = true;
        }
      }
    } else object.flags &= static_cast<uint16_t>(~6u);
  }
  if(object.has(world::object_flag::special)) ++object.timer;
  if((object.flags & 4u) == 0) {
    auto const phase{static_cast<maths::angle>(context.timer * parameters.weave_frequency)};
    object.attitude.heading += static_cast<maths::angle>(maths::sine(phase) / parameters.weave_divisor);
  }
  int speed{near ? parameters.near_speed : parameters.far_speed};
  if(near && (object.flags & 2u) != 0) {
    speed /= 4;
    if((object.flags & 4u) != 0) speed = 0;
  }
  object.velocity = maths::transform(maths::make_inverse_rotation(object.attitude), {maths::word(speed), 0, 0});
}
void actors::update_alien(world::placed_object &object, actor_context const &context, flight::time_step const /*step*/) {
  /// Aliens activate around the on-foot pilot and hop with quality-specific gravity and horizontal drift
  ++object.timer;
  if((object.flags & 4u) == 0) return;
  if(context.on_foot) {
    auto const delta{vec3<int64_t>{object.position} - vec3<int64_t>{context.observer}};
    if(std::max({std::abs(delta.x), std::abs(delta.y), std::abs(delta.z)}) < 32'768) {
      object.timer = 16'000;
      object.flags |= 8u;
    }
  }
  if((object.timer & 16'384u) != 0 || !object.has(world::object_flag::collision_callback) || object.position.z > 0) return;
  auto const &parameters{resources::alien_qualities[object.quality()]};
  object.position.z = 0;
  object.velocity.z = maths::word(parameters.jump_velocity);
  object.motion.gravity = maths::word(parameters.gravity);
  auto hop{[&](int16_t const previous, uint16_t const span){
    int16_t const value{maths::word(previous + random.below(span) - span / 2)};
    return maths::word(value - (value >> 3));
  }};
  object.velocity.x = hop(object.velocity.x, parameters.horizontal_jitter);
  object.velocity.y = hop(object.velocity.y, 50);
  object.attitude.heading = static_cast<maths::angle>(hop(maths::word(object.attitude.heading), 1000));
}
void actors::update_balloon(world::placed_object &object, flight::time_step const step) {
  /// Barrage balloons bounce vertically and occasionally reverse while bees also randomise all three angles
  object.timer = 32'000;
  auto const &parameters{resources::balloon_qualities[object.quality()]};
  int16_t const speed{timed(maths::word(parameters.vertical_launch_speed), step)};
  if(object.velocity.z == 0 || object.position.z < 16'383) object.velocity.z = speed;
  else if(object.position.z > 65'535) object.velocity.z = maths::negate(speed);
  else if(random.next() <= static_cast<uint16_t>(timed(maths::word(parameters.reversal_threshold), step))) object.velocity.z = maths::negate(object.velocity.z);
  if(object.type == world::object_type::bee_swarm) object.attitude = {
    .heading{random.next()},
    .pitch{random.next()},
    .roll{random.next()},
  };
}
void actors::update_orb(world::placed_object &object, actor_context const &context) {
  /// Orb objectives consume the preceding active census; destruction leaves the objective anchor in place
  object.timer = 32'000;
  if(object.quality() == 4) object.health = 65;
  if(!objective_in_range(object, geometry.definition(object.type), context.observer)) return;
  auto population{[&](world::object_type const type){return previous_counts[std::to_underlying(type)];}};
  using enum world::object_type;
  bool complete{false};
  switch(object.quality()) {
  case 0:
  case 4: complete = object.type == destroyed_generator_orb; break;
  case 1: complete = population(fuel_dump) + population(tet) == 0; break;
  case 2: complete = population(kla_site) + population(aaa_site) == 0; break;
  case 3: complete = population(ground_vehicle) + population(vehicle_site) == 0; break;
  case 5: complete = population(saucer) + population(objective_saucer) + population(saucer_site) == 0; break;
  case 6: complete = population(mindbender) == 0; break;
  case 7: complete = observed_aliens != 0 && population(alien) == 0; break;
  }
  if(complete) weapons.complete(object);
}
actor_effects const &actors::advance(actor_context const &context, flight::time_step const step) {
  /// Run the original nearby object handlers and rebuild the census for the following pass
  effects = {};
  std::array<unsigned int, std::to_underlying(world::object_type::count)> counts{};
  for(unsigned int i{0}; i != theatre.objects.size(); ++i) {
    auto &object{theatre.objects[i]};
    if(!object.has(world::object_flag::present) || !world::in_active_neighbourhood(object.position, context.observer)) continue;
    auto const &definition{geometry.definition(object.type)};
    if(!object.has(world::object_flag::updates)) continue;
    if(i < weapons.stored_objects()) {
      if(object.timer != 0) --object.timer;
      if(definition.motion.spin != maths::fixed_vector{0, 0, 0} || definition.motion.gravity != 0 || object.velocity != maths::fixed_vector{0, 0, 0})
        world::advance_motion(object, definition.rotate_below_tier != 0, random, step);
    }
    ++counts[std::to_underlying(object.type)];
    using enum world::object_type;
    switch(object.type) {
    case kla_site:
    case vehicle_site:
    case saucer_site: update_launcher(object, context, step); break;
    case kla: update_kla(object, context, step); break;
    case aaa_site: update_aaa(object, context, step); break;
    case aa_shell: update_shell(i, context, step); break;
    case saucer:
    case objective_saucer: update_saucer(object, context, step); break;
    case ground_vehicle:
    case rescue_van: update_vehicle(object, context, step); break;
    case alien: update_alien(object, context, step); break;
    case barrage_balloon:
    case bee_swarm: update_balloon(object, step); break;
    case generator_orb:
    case destroyed_generator_orb: update_orb(object, context); break;
    case mindbender:
      ++object.timer;
      if(auto const target{relative_target(object.position, context.observer)}; target && target->distance <= 1000) {
        int const proximity{1000 - target->distance};
        effects.hallucination = static_cast<uint16_t>(proximity * proximity / 31);
      }
      break;
    case fuel_dump:
    case control_tower:
    case radar_dish:
    case tet:
      ++object.timer;
      break;
    default:
      break;
    }
    if((definition.properties & std::to_underlying(world::object_property::forcefield)) != 0) {
      if(previous_counts[std::to_underlying(generator_orb)] == 0 && counts[std::to_underlying(destroyed_generator_orb)] != 0) object.flags = 0;
      else object.timer = 32'000;
    }
  }
  observed_aliens = std::max(observed_aliens, counts[std::to_underlying(world::object_type::alien)]);
  previous_counts = counts;
  return effects;
}

} // namespace corncob::game
