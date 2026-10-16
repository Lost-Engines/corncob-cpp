#pragma once

#include <span>
#include <string_view>
#include "resources/collection.h"

namespace corncob::graphics {

class interface_canvas {
private:
  render::framebuffer &target;
  std::span<uint8_t const> font;
  unsigned int font_height;

public:
  interface_canvas(render::framebuffer &target, std::span<uint8_t const> font, unsigned int font_height, unsigned int width = 640, unsigned int height = 350);
  void clear(render::rgba_pixel colour = {0, 0, 0});
  void rectangle(int left, int top, int right, int bottom, render::rgba_pixel colour);
  void box(int left, int top, int right, int bottom, render::rgba_pixel colour);
  void text(int column, int row, std::string_view text, render::rgba_pixel colour, unsigned int scale = 1);
  void image(resources::indexed_image const &image, int x, int y);
};

void draw_presentation(render::framebuffer &target, resources::collection const &resources, resources::presentation_panel const &panel);

} // namespace corncob::graphics
