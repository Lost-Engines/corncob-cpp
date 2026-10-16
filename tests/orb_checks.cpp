#include <array>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include "game/actors.h"
#include "resources/assets.h"
#include "tests/actor_record.h"

int main(int const argc, char const *const argv[]) {
  /// Compare original objective callbacks, dead-orb continuation and proximity effects without substituting native game logic
  if(argc != 2) throw std::invalid_argument{"orb fixture required"};
  std::ifstream file{argv[1], std::ios::binary};
  std::vector<uint8_t> const bytes{std::istreambuf_iterator<char>{file}, {}};
  corncob::resources::binary_reader const data{bytes};
  if(bytes.size() != 163'596 || std::string{bytes.begin(), bytes.begin() + 8} != "CORNORB2") throw std::runtime_error{"invalid orb fixture"};
  namespace game = corncob::game;
  namespace world = corncob::world;
  namespace maths = corncob::maths;
  std::array<world::object_type, 10> constexpr census_types{world::object_type::fuel_dump, world::object_type::tet,
    world::object_type::kla_site, world::object_type::aaa_site, world::object_type::ground_vehicle, world::object_type::vehicle_site,
    world::object_type::saucer, world::object_type::saucer_site, world::object_type::mindbender, world::object_type::alien};
  corncob::resources::geometry_bank geometry{corncob::resources::engine_data_asset()};
  for(unsigned int i{0}, offset{12}; i != 768; ++i, offset += 213) {
    auto const initial{corncob::tests::read_actor(data, offset)};
    maths::world_position const observer{data.signed_dword(offset + 74), data.signed_dword(offset + 78), data.signed_dword(offset + 82)};
    auto const velocity{data.vector(offset + 86)};
    world::theatre theatre;
    game::combat weapons{geometry, theatre, data.dword(offset + 117)};
    game::actors actors{geometry, theatre, weapons};
    for(unsigned int n{0}; n != data.word(offset + 115); ++n) {
      theatre.objects[n].type = world::object_type::alien;
      theatre.objects[n].flags = 0x11;
    }
    actors.refresh_census(observer);
    for(auto &entry : theatre.objects) entry.flags = 0;
    unsigned int cursor{0};
    for(unsigned int type{0}; type != census_types.size(); ++type) for(unsigned int n{0}; n != data.word(offset + 95 + type * 2); ++n) {
      theatre.objects[cursor].type = census_types[type];
      theatre.objects[cursor++].flags = 0x11;
    }
    actors.refresh_census(observer);
    for(auto &entry : theatre.objects) entry.flags = 0;
    unsigned int const main_index{data.byte(offset + 121) != 0 ? 1u : 0u};
    if(main_index != 0) theatre.objects[0] = {
      .position{maths::world_position{observer}},
      .attitude{},
      .type{world::object_type::generator_orb},
      .flags{geometry.definition(world::object_type::generator_orb).flags},
      .timer{32'000},
      .health{65},
    };
    theatre.objects[main_index] = initial;
    auto const &effects{actors.advance({observer, observer, velocity, data.byte(offset + 94) != 0, 0}, {data.signed_word(offset + 92), 5})};
    corncob::tests::compare_actor(theatre.objects[main_index], corncob::tests::read_actor(data, offset + 122), i, "orb/mindbender");
    if(weapons.random_stream().seed() != data.dword(offset + 202)) throw std::runtime_error{"native orb PRNG continuation " + std::to_string(i)};
    if(weapons.stats().objectives_completed != data.word(offset + 206)) throw std::runtime_error{"native objective completion " + std::to_string(i)};
    auto const expected_velocity{data.vector(offset + 196)};
    for(unsigned int axis{0}; axis != 3; ++axis)
      if(maths::word(velocity[axis] + effects.attraction[axis]) != expected_velocity[axis]) throw std::runtime_error{"native orb attraction " + std::to_string(i)};
    uint16_t const distortion{static_cast<uint16_t>((data.byte(offset + 208) & 2u) != 0 ? data.word(offset + 209) : 0)};
    uint16_t const hallucination{static_cast<uint16_t>((data.byte(offset + 208) & 1u) != 0 ? data.word(offset + 211) : 0)};
    if(effects.orb_distortion != distortion || effects.hallucination != hallucination) throw std::runtime_error{"native orb/mindbender visual effects " + std::to_string(i)};
  }
  {
    /// Mutated motion must reach a stored world object, then remain stable at the native resting height across later frames
    world::theatre theatre;
    theatre.objects.push_back({
      .position{0, 0, 40'000},
      .attitude{},
      .type{world::object_type::destroyed_generator_orb},
      .flags{static_cast<uint16_t>(geometry.definition(world::object_type::destroyed_generator_orb).flags | 0x200u)},
      .timer{32'000},
    });
    game::combat weapons{geometry, theatre};
    game::actors actors{geometry, theatre, weapons};
    maths::world_position const observer{0, 0, 0};
    maths::fixed_vector const velocity{0, 0, 0};
    game::actor_context const context{observer, observer, velocity, false, 0};
    actors.advance(context, {});
    actors.advance(context, {});
    if(theatre.objects[0].position.z >= 40'000 || theatre.objects[0].velocity.z >= 0) throw std::runtime_error{"completed stored orb did not begin falling"};
    for(unsigned int frame{0}; frame != 200; ++frame) actors.advance(context, {});
    if(theatre.objects[0].position.z != 3100 || theatre.objects[0].velocity != maths::fixed_vector{0, 0, 0}) throw std::runtime_error{"completed stored orb failed to settle at its native resting height"};
  }
  std::cout << "768 original orb/objective and mindbender updates match without stubs" << std::endl;
  return EXIT_SUCCESS;
}
