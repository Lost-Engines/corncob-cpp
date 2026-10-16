#pragma once

#include "graphics/raster.h"
#include "maths/fixed.h"

namespace corncob::graphics {

void homing_pointer(indexed_framebuffer &target, random_source &random, maths::world_position const &home,
                    maths::world_position const &observer, maths::angle heading, int view_bottom);

} // namespace corncob::graphics
