#include "map_projection.h"
#include <cstdlib>
#include "maths/integer.h"

namespace corncob::graphics {

maths::world_position map_relative(maths::world_position const &difference) noexcept {
  /// REL3D caps apparent depth at 35 high words in map mode, scaling all components while keeping model vertices at their authored sizes
  auto result{maths::world_position{difference}};
  if(difference.z >= 0 || -(difference.z >> 16) <= 35) return result;
  uint32_t const depth{static_cast<uint32_t>(std::abs(static_cast<int64_t>(difference.z))) >> 12};
  if(depth <= 35 * 16) return result;
  uint32_t const factor{((35u * 16u * 65'536u) / depth) >> 1};
  for(unsigned int axis{0}; axis != 3; ++axis) result[axis] = maths::dword(((static_cast<int64_t>(difference[axis]) * factor) >> 16) * 2);
  return result;
}

} // namespace corncob::graphics
