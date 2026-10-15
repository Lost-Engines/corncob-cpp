#include <array>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include "graphics/scene.h"
#include "resources/assets.h"

int main() {
  /// Exercise every theatre across octants, near-plane intersections and cockpit/rear view layouts
  corncob::resources::geometry_bank geometry{corncob::resources::engine_data_asset()};
  auto const cockpit{corncob::resources::decode_cockpit_mask(corncob::resources::cockpit_asset())};
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
        auto const drawn{renderer.render(theatre, observer, frames++, 1024, output)};
        objects += drawn.objects;
        polygons += drawn.polygons;
        discs += drawn.discs;
      }
    }
  }
  if(objects == 0 || polygons == 0 || discs == 0) throw std::runtime_error{"theatre sweep failed to exercise the scene renderer"};
  std::cout << "Rendered " << frames << " original-theatre views: " << objects << " objects, " << polygons
    << " polygons, " << discs << " discs" << std::endl;
  return EXIT_SUCCESS;
}
