#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include "game/actors.h"
#include "resources/assets.h"
#include "resources/binary_reader.h"
#include "tests/actor_record.h"

int main(int const argc, char const *const argv[]) {
  /// Supply a preceding census through the public actor API and compare unchanged original site/constructor executions
  if(argc != 2) throw std::invalid_argument{"launcher fixture required"};
  std::ifstream file{argv[1], std::ios::binary};
  std::vector<uint8_t> const bytes{std::istreambuf_iterator<char>{file}, {}};
  corncob::resources::binary_reader const data{bytes};
  if(bytes.size() != 182'796 || std::string{bytes.begin(), bytes.begin() + 8} != "CORNLAU1") throw std::runtime_error{"invalid launcher fixture"};
  namespace game = corncob::game;
  namespace world = corncob::world;
  corncob::resources::geometry_bank geometry{corncob::resources::engine_data_asset()};
  unsigned int launches{0};
  for(unsigned int i{0}, offset{12}; i != 768; ++i, offset += 238) {
    auto const initial{corncob::tests::read_actor(data, offset)};
    unsigned int const sites{data.word(offset + 74)}, population{data.word(offset + 76)};
    bool const full{data.byte(offset + 84) != 0};
    world::object_type const child{initial.type == world::object_type::kla_site ? world::object_type::kla
      : initial.type == world::object_type::vehicle_site ? world::object_type::ground_vehicle : world::object_type::saucer};
    world::theatre theatre;
    game::combat weapons{geometry, theatre, data.dword(offset + 80)};
    game::actors actors{geometry, theatre, weapons};
    for(unsigned int n{0}; n != 320; ++n) weapons.spawn(world::object_type::aircraft, {0, 0, 0});
    for(auto &entry : theatre.objects) entry.flags = 0;
    for(unsigned int n{0}; n != sites + population; ++n) {
      theatre.objects[n].type = n < sites ? initial.type : child;
      theatre.objects[n].flags = 0x11;
    }
    corncob::maths::world_position const origin{0, 0, 0};
    actors.refresh_census(origin);
    for(auto &entry : theatre.objects) entry.flags = full ? 1 : 0;
    // the native constructor runs now; the new actor enters the painter tables on the following pass
    theatre.objects[0] = initial;
    corncob::maths::fixed_vector const velocity{0, 0, 0};
    actors.advance({origin, origin, velocity, false, 0}, {data.signed_word(offset + 78), 5});
    corncob::tests::compare_actor(theatre.objects[0], corncob::tests::read_actor(data, offset + 85), i, "site");
    bool const expected_spawn{data.byte(offset + 163) != 0};
    bool const spawned{!full && theatre.objects[1].flags != 0};
    if(spawned != expected_spawn) throw std::runtime_error{"native launcher allocation case " + std::to_string(i)};
    if(weapons.random_stream().seed() != data.dword(offset + 159)) throw std::runtime_error{"native launcher random continuation " + std::to_string(i)};
    if(spawned) { corncob::tests::compare_actor(theatre.objects[1], corncob::tests::read_actor(data, offset + 164), i, "child"); ++launches; }
  }
  std::cout << "768 original site updates and " << launches << " complete actor constructors match" << std::endl;
  return EXIT_SUCCESS;
}
