#pragma once

#include <array>
#include <cstdint>
#include <span>
#include "render/framebuffer.h"

namespace corncob::resources {

using cockpit_mask = std::array<uint8_t, render::screen_layout::mask_bytes>;

cockpit_mask decode_cockpit_mask(std::span<uint8_t const> source);
void draw_cockpit_preview(cockpit_mask const &mask, render::framebuffer &target) noexcept;

} // namespace corncob::resources
