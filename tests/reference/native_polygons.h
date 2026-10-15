#pragma once

#include <array>
#include <cstdint>
#include "graphics/raster.h"

namespace corncob::tests {

struct native_polygon {
  std::array<graphics::screen_point, 20> vertices;
  unsigned int count;
  uint64_t hash;
};

inline std::array<native_polygon, 12> const native_polygons{
  native_polygon{
    .vertices{
      graphics::screen_point{100, 50},
      graphics::screen_point{200, 70},
      graphics::screen_point{150, 150},
    },
    .count{3},
    .hash{0x6b49460c2ef71815ull},
  },
  native_polygon{
    .vertices{
      graphics::screen_point{150, 150},
      graphics::screen_point{200, 70},
      graphics::screen_point{100, 50},
    },
    .count{3},
    .hash{0x6b49460c2ef71815ull},
  },
  native_polygon{
    .vertices{
      graphics::screen_point{120, 40},
      graphics::screen_point{500, 40},
      graphics::screen_point{500, 180},
      graphics::screen_point{120, 180},
    },
    .count{4},
    .hash{0x9e327c6dab7d35e5ull},
  },
  native_polygon{
    .vertices{
      graphics::screen_point{0, 0},
      graphics::screen_point{639, 0},
      graphics::screen_point{639, 349},
      graphics::screen_point{0, 349},
    },
    .count{4},
    .hash{0xd886d627486750a5ull},
  },
  native_polygon{
    .vertices{
      graphics::screen_point{-200, 25},
      graphics::screen_point{320, -100},
      graphics::screen_point{620, 180},
      graphics::screen_point{100, 150},
    },
    .count{4},
    .hash{0xa8de5749b54913f1ull},
  },
  native_polygon{
    .vertices{
      graphics::screen_point{100, 50},
      graphics::screen_point{101, 50},
      graphics::screen_point{150, 180},
    },
    .count{3},
    .hash{0xd4cc416d5446cd39ull},
  },
  native_polygon{
    .vertices{
      graphics::screen_point{100, 50},
      graphics::screen_point{150, 50},
      graphics::screen_point{200, 50},
    },
    .count{3},
    .hash{0x45cbb1ea5eceff25ull},
  },
  native_polygon{
    .vertices{
      graphics::screen_point{100, 50},
      graphics::screen_point{200, 50},
      graphics::screen_point{220, 100},
      graphics::screen_point{200, 150},
      graphics::screen_point{100, 150},
      graphics::screen_point{80, 100},
    },
    .count{6},
    .hash{0xca2e6b0560159805ull},
  },
  native_polygon{
    .vertices{
      graphics::screen_point{100, 70},
      graphics::screen_point{200, 60},
      graphics::screen_point{300, 70},
      graphics::screen_point{320, 160},
      graphics::screen_point{180, 120},
    },
    .count{5},
    .hash{0x5244fa0651190aadull},
  },
  native_polygon{
    .vertices{
      graphics::screen_point{20, -300},
      graphics::screen_point{300, 10},
      graphics::screen_point{620, 180},
      graphics::screen_point{10, 180},
    },
    .count{4},
    .hash{0xa0d417fa77bd045dull},
  },
  native_polygon{
    .vertices{
      graphics::screen_point{8, 4},
      graphics::screen_point{631, 4},
      graphics::screen_point{631, 199},
      graphics::screen_point{8, 199},
    },
    .count{4},
    .hash{0xd886d627486750a5ull},
  },
  native_polygon{
    .vertices{
      graphics::screen_point{120, 40},
      graphics::screen_point{320, 150},
      graphics::screen_point{500, 40},
      graphics::screen_point{320, 60},
    },
    .count{4},
    .hash{0x7dd75e22b70f1b0dull},
  },
};

} // namespace corncob::tests
