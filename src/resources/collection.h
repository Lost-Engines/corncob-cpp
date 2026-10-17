#pragma once

#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include "render/framebuffer.h"

namespace corncob::resources {

struct indexed_image {
  unsigned int width;
  unsigned int height;
  std::vector<uint8_t> pixels;
  std::array<render::rgba_pixel, 256> palette;
};
struct panel_text {
  unsigned int row;
  unsigned int column;
  std::string text;
};
struct presentation_panel {
  std::string image;
  int x;
  int y;
  unsigned int text_colour;
  std::vector<panel_text> lines;
};

class collection {
private:
  std::unordered_map<std::string_view, std::span<uint8_t const>> entries;
  collection const *fallback;

public:
  explicit collection(std::span<uint8_t const> source, collection const *fallback = nullptr);
  std::span<uint8_t const> file(std::string_view name) const;
  std::string_view text(std::string_view name) const;
  std::array<uint8_t, 4096> menu_font() const;
  indexed_image image(std::string_view name) const;
  std::vector<presentation_panel> story() const;
  std::vector<presentation_panel> outcomes() const;
};

} // namespace corncob::resources
