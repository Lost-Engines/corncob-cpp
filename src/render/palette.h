#pragma once

#include "render/framebuffer.h"

namespace corncob::render {

inline std::array<rgba_pixel, 16> constexpr ega_palette{
  rgba_pixel{
    .red{0},
    .green{0},
    .blue{0},
  },
  rgba_pixel{
    .red{0},
    .green{0},
    .blue{170},
  },
  rgba_pixel{
    .red{0},
    .green{170},
    .blue{0},
  },
  rgba_pixel{
    .red{0},
    .green{170},
    .blue{170},
  },
  rgba_pixel{
    .red{170},
    .green{0},
    .blue{0},
  },
  rgba_pixel{
    .red{170},
    .green{0},
    .blue{170},
  },
  rgba_pixel{
    .red{170},
    .green{85},
    .blue{0},
  },
  rgba_pixel{
    .red{170},
    .green{170},
    .blue{170},
  },
  rgba_pixel{
    .red{85},
    .green{85},
    .blue{85},
  },
  rgba_pixel{
    .red{85},
    .green{85},
    .blue{255},
  },
  rgba_pixel{
    .red{85},
    .green{255},
    .blue{85},
  },
  rgba_pixel{
    .red{85},
    .green{255},
    .blue{255},
  },
  rgba_pixel{
    .red{255},
    .green{85},
    .blue{85},
  },
  rgba_pixel{
    .red{255},
    .green{85},
    .blue{255},
  },
  rgba_pixel{
    .red{255},
    .green{255},
    .blue{85},
  },
  rgba_pixel{
    .red{255},
    .green{255},
    .blue{255},
  },
};

void fade_palette(std::array<rgba_pixel, 16> &colours, uint8_t intensity) noexcept;

} // namespace corncob::render
