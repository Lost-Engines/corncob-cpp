#pragma once

#include <optional>
#include <span>
#include <string>
#include <vector>
#include "render/framebuffer.h"
#include "world/world.h"

namespace corncob::game {

class tower_briefing {
public:
  static unsigned int constexpr visible_lines{22};
  static unsigned int constexpr line_width{77};
  static unsigned int constexpr airfield_count{9};
  static unsigned int constexpr central_airfield{4};
  static int constexpr half_page{(visible_lines + 1) / 2};
  static int constexpr full_page{visible_lines - 2};

private:
  std::vector<std::string> text_lines;
  unsigned int first{0};
  unsigned int airfield{central_airfield};

public:
  tower_briefing() = default;
  explicit tower_briefing(std::string_view text, unsigned int airfield = central_airfield);
  void move(int lines);
  void home();
  void end();
  unsigned int first_line() const noexcept;
  unsigned int maximum_scroll() const noexcept;
  std::span<std::string const> lines() const noexcept;
  std::optional<world::mission_start> selected_start() const;
  void render(render::framebuffer &target, std::span<uint8_t const> font) const;
};

} // namespace corncob::game
