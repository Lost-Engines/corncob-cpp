#include "scenery.h"
#include "maths/integer.h"

namespace corncob::game {

std::optional<uint16_t> interaction_range(maths::world_position const &position, maths::world_position const &observer) noexcept {
  /// EVALDIST accepts local word coordinates and uses the original integer magnitude for on-foot contacts
  maths::fixed_vector relative{0, 0, 0};
  for(unsigned int axis{0}; axis != 3; ++axis) {
    int32_t const delta{maths::dword(static_cast<int64_t>(position[axis]) - observer[axis])};
    uint32_t const absolute{delta < 0 ? 0u - static_cast<uint32_t>(delta) : static_cast<uint32_t>(delta)};
    // REL3D tests the high word after doubling; retain its dword wrap at the most-negative displacement
    if(static_cast<uint16_t>((absolute << 1) >> 16) != 0) return std::nullopt;
    relative[axis] = maths::word(delta);
  }
  return maths::magnitude(relative);
}

scenery_event advance_scenery(world::placed_object &object, maths::world_position const &observer, bool const on_foot, uint32_t const fast_ticks) noexcept {
  /// Permanent scenery refreshes its timer and retains one-way pose changes and local pilot interactions
  uint16_t constexpr permanent_timer{32'000};
  auto const contact{[&]{
    auto const distance{interaction_range(object.position, observer)};
    return on_foot && distance && *distance <= contact_distance;
  }};
  switch(object.type) {
  case world::object_type::ground_crew:
    object.timer = permanent_timer;
    if((object.flags & 0x1000u) != 0) {
      object.pose = world::model_pose::shot;
      object.attitude.pitch = 16'384;
    } else if(object.pose == world::model_pose::normal && contact()) {
      object.pose = world::model_pose::gesture;
      if(object.has(world::object_flag::special)) return scenery_event::contact;
    }
    break;
  case world::object_type::tower_doorway:
    object.timer = permanent_timer;
    if(contact()) return scenery_event::tower;
    break;
  case world::object_type::radar_dish:
    ++object.timer;
    object.attitude.heading = static_cast<maths::angle>(fast_ticks * 4u);
    break;
  case world::object_type::runway_piece:
    object.timer = permanent_timer;
    if(object.quality() != 0) object.pose = world::model_pose::paved;
    break;
  case world::object_type::radar_base:
  case world::object_type::propeller:
  case world::object_type::destroyed_fuel_dump:
  case world::object_type::runway_end:
  case world::object_type::runway_end_1:
  case world::object_type::fence_1:
  case world::object_type::fence_1a:
  case world::object_type::fence_1b:
    object.timer = permanent_timer;
    break;
  default:
    break;
  }
  return scenery_event::none;
}

} // namespace corncob::game
