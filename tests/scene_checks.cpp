#include <array>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include "graphics/scene.h"
#include "resources/assets.h"
#include "tests/reference/native_tower.h"
#include "tests/reference_reader.h"

namespace {

uint64_t scene_hash(corncob::graphics::scene_renderer const &renderer) {
  /// Normalize only the horizon colours before hashing the native probe's scenery region
  uint64_t hash{14'695'981'039'346'656'037ull};
  uint64_t constexpr fnv_prime{1'099'511'628'211ull};
  uint8_t constexpr ground_colour{2}, sky_colour{11};
  auto const &crop{corncob::tests::native_tower_crop};
  for(int y{crop.top}; y != crop.bottom; ++y) {
    for(int x{crop.left}; x != crop.right; ++x) {
      auto const colour{renderer.indexed_frame().pixels[static_cast<unsigned int>(y) * corncob::render::screen_layout::width + static_cast<unsigned int>(x)]};
      // the native probe clears to one flat colour; compare scenery independently of the horizon
      hash = (hash ^ (colour == sky_colour ? ground_colour : colour)) * fnv_prime;
    }
  }
  return hash;
}

void check_tower(corncob::resources::geometry_bank &geometry, corncob::resources::cockpit_mask const &cockpit) {
  /// Compare native scene regions, including the original door showing through the side and back walls
  namespace world = corncob::world;
  namespace graphics = corncob::graphics;
  world::theatre theatre;
  theatre.objects.push_back({
    .position{0, 0, 16'000},
    .attitude{},
    .type{world::object_type::control_tower},
    .flags{1},
  });
  theatre.objects.push_back({
    .position{0, -2000, 0},
    .attitude{},
    .type{world::object_type::tower_doorway},
    .flags{1},
  });
  graphics::scene_renderer renderer{geometry, cockpit};
  corncob::render::framebuffer output;
  for(auto const &view : corncob::tests::native_tower_views) {
    graphics::camera const observer{
      .position{view.x, view.y, 720},
      .attitude{
        .heading{view.heading},
      },
      .cockpit{false},
      .rear_view{false},
      .ground_detail{false},
    };
    theatre.objects.back().flags = view.doorway ? 1 : 0;
    corncob::maths::random_source random;
    renderer.render(theatre, observer, 0, 0, random, output);
    auto const hash{scene_hash(renderer)};
    if(hash != view.hash) throw std::runtime_error{"tower/doorway scene differs from the native painter order"};
  }
}

void check_runways(corncob::resources::geometry_bank &geometry, corncob::resources::cockpit_mask const &cockpit, char const *const path) {
  /// Original DRAWNEAR paints only zero-altitude runway pieces before its ordinary object pass
  namespace world = corncob::world;
  corncob::tests::reference_reader input{path, "CORNSCN1"};
  auto const cases{input.dword()};
  corncob::graphics::scene_renderer renderer{geometry, cockpit};
  corncob::render::framebuffer output;
  for(unsigned int i{0}; i != cases; ++i) {
    auto const type{static_cast<world::object_type>(input.word())};
    corncob::graphics::camera const observer{
      .position{input.signed_dword(), input.signed_dword(), input.signed_dword()},
      .attitude{
        .heading{input.word()},
      },
      .cockpit{false},
      .rear_view{false},
      .ground_detail{false},
    };
    int32_t const altitude{input.signed_dword()};
    uint64_t const low{input.dword()};
    uint64_t const expected{low | (static_cast<uint64_t>(input.dword()) << 32)};
    world::theatre theatre;
    theatre.objects.push_back({
      .position{0, 0, 16'000},
      .attitude{},
      .type{world::object_type::control_tower},
      .flags{1},
    });
    theatre.objects.push_back({
      .position{0, 0, altitude},
      .attitude{},
      .type{type},
      .flags{1},
    });
    corncob::maths::random_source random;
    renderer.render(theatre, observer, 0, 0, random, output);
    if(scene_hash(renderer) != expected) throw std::runtime_error{"native runway scene " + std::to_string(i)};
  }
}

} // anonymous namespace

int main(int const argc, char const *const argv[]) {
  if(argc != 2) throw std::invalid_argument{"native scene-order fixture required"};
  /// Exercise every theatre across octants, near-plane intersections and cockpit/rear view layouts
  corncob::resources::geometry_bank geometry{corncob::resources::engine_data_asset()};
  auto const cockpit{corncob::resources::decode_cockpit_mask(corncob::resources::cockpit_asset())};
  check_tower(geometry, cockpit);
  check_runways(geometry, cockpit, argv[1]);
  corncob::maths::random_source scene_random;
  corncob::graphics::scene_renderer renderer{geometry, cockpit};
  corncob::render::framebuffer output;
  unsigned int frames{0}, objects{0}, polygons{0}, discs{0};
  for(auto const data : {corncob::resources::white_sands_asset(), corncob::resources::morning_asset(),
                        corncob::resources::dusk_asset(), corncob::resources::wasteland_asset()}) {
    auto const theatre{corncob::world::decode_theatre(data)};
    corncob::graphics::camera observer{
      .position{corncob::maths::world_position{theatre.start}},
      .attitude{},
    };
    for(auto const &object : theatre.objects) {
      observer.position = corncob::maths::world_position{object.position};
      observer.position.x -= 800;
      observer.position.z += 1500;
      for(unsigned int quadrant{0}; quadrant != 4; ++quadrant) {
        observer.attitude = {
          .heading{static_cast<corncob::maths::angle>(quadrant * 16'384)},
          .pitch{static_cast<corncob::maths::angle>(quadrant * 2048)},
          .roll{static_cast<corncob::maths::angle>(quadrant * 8192)},
        };
        observer.cockpit = (quadrant & 1u) == 0;
        observer.rear_view = quadrant == 0;
        auto const drawn{renderer.render(theatre, observer, frames++, 1024, scene_random, output)};
        objects += drawn.objects;
        polygons += drawn.polygons;
        discs += drawn.discs;
      }
    }
  }
  if(objects == 0 || polygons == 0 || discs == 0) throw std::runtime_error{"theatre sweep failed to exercise the scene renderer"};
  std::cout << "64 original tower/doorway/runway regions match. Rendered " << frames << " original-theatre views: " << objects << " objects, " << polygons
    << " polygons, " << discs << " discs" << std::endl;
  return EXIT_SUCCESS;
}
