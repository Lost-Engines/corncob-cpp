#pragma once

#include <string>
#include <vector>

namespace corncob::resources {

enum class radio_notice {
  choose_mission,
  training,
  objective_complete,
  objective_damaged,
  jump_damaged,
  no_training_map,
  map_instructions,
  low_altitude,
  leave_aircraft,
  leave_on_foot,
  ground_steering,
  count,
};
struct message_box {
  int left;
  int top;
  int width;
  int height;
};
struct message_line {
  int column;
  int row;
  std::string text;
};
struct radio_message {
  message_box border;
  message_box background;
  std::vector<message_line> lines;
};

radio_message read_radio_message(radio_notice notice);

} // namespace corncob::resources
