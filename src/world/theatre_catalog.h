#pragma once

#include <array>
#include <string_view>

namespace corncob::world {

struct theatre_definition {
  std::string_view name;
  std::string_view title;
  std::string_view dos_name;
  unsigned int reserve_aircraft;
};

/// The four base-game definitions and initial reserve counts retained in MOAG.CFG's original definition table
inline std::array<theatre_definition, 4> constexpr theatre_catalog{
  theatre_definition{
    .name{"morning"},
    .title{"Atkinsville (easy)"},
    .dos_name{"morning"},
    .reserve_aircraft{7},
  },
  theatre_definition{
    .name{"dusk"},
    .title{"Dusk (medium)"},
    .dos_name{"dusk"},
    .reserve_aircraft{6},
  },
  theatre_definition{
    .name{"wasteland"},
    .title{"Wasteland (medium)"},
    .dos_name{"wastelnd"},
    .reserve_aircraft{6},
  },
  theatre_definition{
    .name{"white_sands"},
    .title{"White Sands Testing Ground"},
    .dos_name{"whtsands"},
    .reserve_aircraft{6},
  },
};

} // namespace corncob::world
