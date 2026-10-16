#pragma once

#include <optional>
#include <string>
#include "game/session.h"
#include "game/campaign.h"
#include "world/training.h"
#include "graphics/interface.h"
#include "resources/cockpit_mask.h"

namespace corncob::game {

enum class interface_command {
  up,
  down,
  previous,
  next,
  accept,
  cancel,
  help,
  page_up,
  page_down,
  backspace,
};
enum class interface_page {
  title,
  story,
  menu,
  theatres,
  training,
  missions,
  roster,
  pilots,
  pilot_name,
  records,
  sortie_summary,
  current_score,
  briefing,
  help,
  quit_confirmation,
  outcome,
  flight,
};
struct launch_request {
  std::string theatre;
  std::optional<world::training_setup> training{};
};

class front_end {
private:
  resources::collection const &assets;
  campaign_store &campaign;
  enum class roster_action { activate, records, rename, resurrect };
  roster_action pilot_action{roster_action::activate};
  unsigned int pilot_index{0};
  std::string entered_name;
  std::string feedback;
  bool renaming{false};
  bool pending_finish{false};
  world::training_setup training;
  std::vector<world::mission_start> mission_starts;
  std::optional<world::mission_start> pending_relocation;
  std::optional<sortie_statistics> latest_result;
  std::vector<resources::presentation_panel> stories;
  std::vector<resources::presentation_panel> outcomes;
  std::vector<resources::cockpit_mask> help_screens;
  interface_page page{interface_page::title};
  interface_page return_page{interface_page::menu};
  unsigned int selection{0};
  unsigned int story_index{0};
  unsigned int help_index{0};
  unsigned int scroll{0};
  unsigned int outcome_index{0};
  std::string selected_theatre;
  std::string briefing;
  std::optional<launch_request> pending_launch;
  bool quit_requested{false};
  bool session_running{false};

  void accept();
  unsigned int choice_count() const;
  void adjust_training(int direction);
  void draw_menu(graphics::interface_canvas &canvas, std::string_view title, std::span<std::string_view const> choices) const;

public:
  front_end(resources::collection const &assets, campaign_store &campaign, bool skip_intro = false, bool start_in_flight = false);
  void character(uint32_t codepoint);
  bool editing_name() const noexcept;
  bool take_finish();
  void finish_sortie(sortie_statistics const &result);
  void command(interface_command command);
  void begin_flight();
  void show_menu();
  void show_intelligence(world::theatre const &world);
  void show_missions(world::theatre const &world);
  void show_current_score(sortie_statistics const &result);
  std::optional<world::mission_start> take_relocation();
  void show_flight_help();
  void ask_to_end_sortie();
  void show_outcome(unsigned int index);
  void render(render::framebuffer &target) const;
  std::optional<launch_request> take_launch();
  bool in_flight() const noexcept;
  bool quitting() const noexcept;
  interface_page current_page() const noexcept;
};

} // namespace corncob::game
