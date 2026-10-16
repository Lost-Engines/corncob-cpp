#pragma once

#include <array>
#include <span>
#include "graphics/instruments.h"
#include "graphics/raster.h"
#include "maths/fixed.h"
#include "render/framebuffer.h"
#include "resources/cockpit_mask.h"
#include "resources/geometry_bank.h"
#include "world/world.h"

namespace corncob::graphics {

struct camera {
  maths::world_position position{0, 0, 720};
  maths::angles attitude;
  bool cockpit{true};
  bool rear_view{true};
  bool ground_detail{true};
  uint16_t orb_distortion{0};
  vec3<uint16_t> palette_phase{0, 0, 0};
};

struct render_statistics {
  unsigned int objects{0};
  unsigned int polygons{0};
  unsigned int lines{0};
  unsigned int discs{0};
};

struct view_parameters {
  clip_rectangle bounds;
  int focal_x;
  int focal_y;
  int centre_x;
  int centre_y;
  bool rear;
};

class scene_renderer {
private:
  resources::geometry_bank &geometry;
  resources::cockpit_mask const &cockpit;
  indexed_framebuffer indexed;
  random_source random;
  std::array<render::rgba_pixel, 16> palette;
  render_statistics statistics;
  instrument_panel instruments;
  std::array<maths::world_position, 80> ground_points;
  bool ground_initialised{false};

  void set_palette(world::theatre const &theatre, maths::world_position const &position);
  void background(view_parameters const &view, maths::rotation const &rotation);
  void draw_ground_and_stars(world::theatre const &theatre, camera const &observer, maths::rotation const &rotation, std::span<view_parameters const> views);
  void draw_model(resources::model_address address, unsigned int octant, maths::rotation const &matrix,
                  maths::fixed_vector const &origin, unsigned int reduction, std::span<view_parameters const> views);

public:
  scene_renderer(resources::geometry_bank &geometry, resources::cockpit_mask const &cockpit);
  render_statistics render(world::theatre const &theatre, camera const &observer, unsigned int frame,
                           uint16_t timer, render::framebuffer &output, instrument_telemetry const *telemetry = nullptr);
};

} // namespace corncob::graphics
