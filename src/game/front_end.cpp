#include "front_end.h"
#include "render/palette.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <format>
#include <utility>
#include "resources/assets.h"
#include "resources/airfield_survey.h"
#include "platform/storage.h"
#include "world/world.h"
#include "world/theatre_catalog.h"

namespace corncob::game {

namespace {

render::rgba_pixel constexpr white{255, 255, 255}, grey{170, 170, 170}, yellow{255, 255, 85}, blue{0, 0, 170};

} // anonymous namespace

front_end::front_end(resources::collection const &assets, campaign_store &campaign, bool const skip_intro, bool const start_in_flight, std::optional<control_setup> const controls, std::optional<bool> const music_enabled) :
  assets{assets},
  campaign{campaign},
  menu_font{assets.menu_font()},
  training{campaign.training_settings()},
  controls{controls.value_or(control_setup{
    .joystick_device{campaign.keyboard_control() ? -2 : -1},
    .beginner_flight{!campaign.active_pilot() || campaign.active_pilot()->beginner_flight()},
  })},
  stories{assets.story()},
  outcomes{assets.outcomes()},
  music_on{music_enabled.value_or(campaign.music_enabled())} {
  /// One state machine replaces the original title, menu, help and outcome helper processes
  page = start_in_flight ? interface_page::flight : skip_intro ? interface_page::menu : interface_page::story;
  saved_theatres = campaign.theatres();
  combat_invulnerable = campaign.combat_invincible();
  comments_enabled = campaign.inflight_comments();
  for(auto const name : {"img/3d2.img", "img/3d3.img", "img/3d4.img", "img/3d5.img", "img/3d6.img",
    "img/3d7.img", "img/3d8.img", "img/3d9.img", "img/3d10.img", "img/3d13.img"}) help_screens.push_back(resources::decode_cockpit_mask(assets.file(name)));
}
void front_end::navigate(interface_page const destination) {
  /// Each original menu remembers its highlighted entry when a child screen closes
  remembered_selection[page] = selection;
  page = destination;
  auto const found{remembered_selection.find(page)};
  selection = found == remembered_selection.end() ? (page == interface_page::menu ? 2u : 0u) : found->second;
}
void front_end::sync_pilot() {
  /// Activating a different career restores that pilot's menu preferences
  training = campaign.training_settings();
  controls.beginner_flight = !campaign.active_pilot() || campaign.active_pilot()->beginner_flight();
  saved_theatres = campaign.theatres();
  combat_invulnerable = campaign.combat_invincible();
  comments_enabled = campaign.inflight_comments();
}
void front_end::document(std::string_view text, unsigned int const section) {
  /// Form feeds separate the original documentation chapters and context-sensitive menu help
  for(unsigned int n{0}; n < section; ++n) {
    auto const split{text.find('\f')};
    if(split == std::string_view::npos) throw std::invalid_argument{"missing original documentation section"};
    text.remove_prefix(split + 1);
  }
  text = text.substr(0, text.find('\f'));
  std::istringstream input{std::string{text}};
  std::string line;
  document_lines.clear();
  while(std::getline(input, line)) {
    if(!line.empty() && line.back() == '\r') line.pop_back();
    document_lines.push_back(line);
  }
  scroll = 0;
}
void front_end::information(unsigned int const section) {
  /// Menu information is separate from F4's original in-flight artwork
  information_return = page;
  information_section = section;
  document(assets.text("moag.inf"), section);
  if(!document_lines.empty()) document_lines.erase(document_lines.begin());
  navigate(interface_page::menu_information);
}
void front_end::notice(std::string message) {
  /// Report a failed operation inside the same menu without discarding its selection or saved state
  information_return = page;
  feedback = std::move(message);
  navigate(interface_page::notice);
}
void front_end::launch_combat() {
  /// A combat flight resumes the active saved theater; selecting a different definition alone does not reset it
  auto const &worlds{saved_theatres};
  auto const active{campaign.active_theatre()};
  if(!active || *active >= worlds.size()) { notice("Sorry, there is no active theater-of-operation.\nOpen or activate a theater before choosing Fly mission."); return; }
  prepared_world = campaign.load_world(worlds[*active].definition);
  prepared_launch = launch_request{
    .theatre{worlds[*active].definition},
    .invincible{combat_invulnerable},
    .comments{comments_enabled},
  };
  navigate(interface_page::starting_airfield);
}
void front_end::accept() {
  /// Reproduce MOAG's menu hierarchy while retaining one native session and save store
  try {
    switch(page) {
    case interface_page::radio:
      radio.reset(); page = interface_page::flight; pending_map = std::exchange(map_after_radio, false); break;
    case interface_page::ending: ending->toggle_warp(); break;
    case interface_page::title: title.reset(); navigate(interface_page::menu); break;
    case interface_page::story: advancing_story = true; break;
    case interface_page::menu:
      if(session_running) {
        if(selection == 0) page = interface_page::flight;
        else if(selection == 1) show_flight_help();
        else ask_to_end_sortie();
      } else switch(selection) {
      case 0: navigate(interface_page::roster); break;
      case 1: navigate(interface_page::theatres); break;
      case 2: navigate(interface_page::training); break;
      case 3: navigate(interface_page::high_score_menu); break;
      case 4:
        document(assets.text("moag.inf"), 9);
        if(!document_lines.empty()) document_lines.erase(document_lines.begin());
        navigate(interface_page::registration); break;
      case 5: navigate(interface_page::control_setup); break;
      case 6: music_on = !music_on; campaign.music_enabled(music_on); break;
      case 7: document_chapter = 0; numeric_input.clear(); document(assets.text("corncob.doc"), 0); navigate(interface_page::documentation); break;
      case 8:
        platform::write_binary(campaign.save_directory() / "CORNCOB.DOC", assets.file("corncob.doc"));
        notice("The original documentation has been saved for printing.\nCORNCOB.DOC is in your save directory."); break;
      case 9: information(1); break;
      case 10: quit_requested = true; break;
      }
      break;
    case interface_page::roster:
      if(selection == 0) { renaming = false; entered_name.clear(); navigate(interface_page::pilot_name); }
      else if(selection == 2) { campaign.deactivate(); sync_pilot(); }
      else if(selection == 7) information(2);
      else if(selection == 8) navigate(interface_page::menu);
      else if(campaign.pilots().empty()) notice("There are no pilots in the list.\nYou will have to create one first.");
      else {
        pilot_action = selection == 1 ? roster_action::activate : selection == 3 ? roster_action::remove
          : selection == 4 ? roster_action::resurrect : selection == 5 ? roster_action::rename : roster_action::records;
        pilot_index = 0;
        if(pilot_action == roster_action::records) { detailed_records = false; navigate(interface_page::records); }
        else { navigate(interface_page::pilots); selection = 0; }
      }
      break;
    case interface_page::pilots:
      pilot_index = selection;
      switch(pilot_action) {
      case roster_action::activate: campaign.activate(pilot_index); sync_pilot(); navigate(interface_page::roster); break;
      case roster_action::remove: navigate(interface_page::pilot_delete); break;
      case roster_action::rename: renaming = true; entered_name = campaign.pilots()[pilot_index].name(); navigate(interface_page::pilot_name); break;
      case roster_action::resurrect:
        if(!campaign.pilots()[pilot_index].unavailable()) notice("This pilot is not dead or captured.\nYou can only resurrect dead or captured pilots.");
        else navigate(interface_page::pilot_resurrect);
        break;
      case roster_action::records: detailed_records = false; navigate(interface_page::records); break;
      }
      break;
    case interface_page::pilot_delete: campaign.delete_pilot(pilot_index); sync_pilot(); navigate(interface_page::roster); break;
    case interface_page::pilot_resurrect: campaign.resurrect(pilot_index); navigate(interface_page::roster); break;
    case interface_page::pilot_name:
      if(renaming) { campaign.rename_pilot(pilot_index, entered_name); navigate(interface_page::roster); }
      else {
        pilot_record const validated{entered_name};
        if(std::ranges::any_of(campaign.pilots(), [&](auto const &pilot){return pilot.name() == validated.name();}))
          throw std::invalid_argument{"Sorry, that pilot already exists."};
        navigate(interface_page::pilot_activate); selection = 0;
      }
      break;
    case interface_page::pilot_activate: campaign.create_pilot(entered_name, selection == 0); sync_pilot(); navigate(interface_page::roster); break;
    case interface_page::records:
    case interface_page::medals: navigate(interface_page::roster); break;
    case interface_page::boss:
    case interface_page::current_score: page = interface_page::flight; break;
    case interface_page::sortie_summary: sync_pilot(); navigate(interface_page::menu); break;
    case interface_page::control_setup:
      if(selection == 0 || selection == 1) {
        controls.joystick_device = selection == 0 ? -2 : -1;
        campaign.keyboard_control(selection == 0);
      } else if(selection == 2 || selection == 3) automatic_centering = selection == 2;
      else if(selection == 4) force_centering = true;
      else if(selection == 5) information(5);
      else navigate(interface_page::menu);
      break;
    case interface_page::training:
      switch(selection) {
      case 0:
        if(!campaign.can_fly()) { notice("This pilot is not available for duty."); break; }
        prepared_launch = launch_request{
          .theatre{"training"},
          .training{training},
          .invincible{training.invincible},
          .comments{comments_enabled},
        };
        navigate(interface_page::mission_briefing);
        break;
      case 1: navigate(interface_page::training_wickedness); break;
      case 2: navigate(interface_page::training_counts); break;
      case 3: training = {}; campaign.training_settings(training); break;
      case 4: training.invincible = !training.invincible; campaign.training_settings(training); break;
      case 5: information(6); break;
      case 6: navigate(interface_page::menu); break;
      }
      break;
    case interface_page::training_wickedness:
    case interface_page::training_counts:
      if(selection == 3) navigate(interface_page::training);
      else { training_editor = page; training_field = selection; numeric_input.clear(); navigate(interface_page::training_value); }
      break;
    case interface_page::training_value:
      if(!numeric_input.empty()) {
        unsigned int const value{static_cast<unsigned int>(std::stoi(numeric_input))};
        bool const quality{training_editor == interface_page::training_wickedness};
        if(value > (quality ? 8u : world::training_defence::maximum_menu_count)) { feedback = quality ? "Enter a value from 0 to 8" : "Enter a value from 0 to 25"; break; }
        auto &defence{training_field == 0 ? training.kla : training_field == 1 ? training.vehicles : training.aaa};
        if(quality) defence.quality = static_cast<int>(value) - 1;
        else defence.count = value;
        campaign.training_settings(training);
      }
      feedback.clear(); navigate(training_editor); break;
    case interface_page::theatres:
      switch(selection) {
      case 0: if(campaign.can_fly()) launch_combat(); else notice("This pilot is not available for duty."); break;
      case 1: navigate(interface_page::new_theatre); break;
      case 2: case 3: case 4:
        if(saved_theatres.empty()) notice("Sorry, this pilot has no open theaters to choose from.");
        else { detailed_records = false; navigate(selection == 2 ? interface_page::open_theatre : selection == 3 ? interface_page::theatre_delete : interface_page::theatre_records); selection = 0; }
        break;
      case 5: combat_invulnerable = !combat_invulnerable; campaign.combat_invincible(combat_invulnerable); break;
      case 6: controls.beginner_flight = !controls.beginner_flight; campaign.beginner_flight(controls.beginner_flight); break;
      case 7: comments_enabled = !comments_enabled; campaign.inflight_comments(comments_enabled); break;
      case 8: information(3); break;
      case 9: navigate(interface_page::menu); break;
      }
      break;
    case interface_page::new_theatre:
      {
        std::array<std::span<uint8_t const>, 4> const data{resources::morning_asset(), resources::dusk_asset(), resources::wasteland_asset(), resources::white_sands_asset()};
        campaign.open_theatre(world::theatre_catalog[selection].name, world::decode_theatre(data[selection]));
        saved_theatres = campaign.theatres();
        navigate(interface_page::theatres);
      }
      break;
    case interface_page::open_theatre: campaign.activate_theatre(selection); navigate(interface_page::theatres); break;
    case interface_page::theatre_delete: campaign.delete_theatre(selection); saved_theatres = campaign.theatres(); navigate(interface_page::theatres); break;
    case interface_page::theatre_records:
      if(detailed_records) { detailed_records = false; navigate(interface_page::theatres); }
      else detailed_records = true;
      break;
    case interface_page::mission_briefing:
    case interface_page::starting_airfield:
      pending_launch = std::exchange(prepared_launch, std::nullopt);
      break;
    case interface_page::airfield_information: navigate(interface_page::starting_airfield); break;
    case interface_page::missions:
      if(selection == 0) { scroll = 0; page = interface_page::briefing; }
      else { if(selection - 1 < mission_starts.size()) pending_relocation = mission_starts[selection - 1]; page = interface_page::flight; }
      break;
    case interface_page::briefing:
      if(session_running) page = interface_page::flight;
      break;
    case interface_page::high_score_menu:
      if(selection < 3) { score_list = selection; navigate(interface_page::high_scores); }
      else if(selection == 3) navigate(interface_page::score_delete);
      else if(selection == 4) information(7);
      else navigate(interface_page::menu);
      break;
    case interface_page::score_delete:
      if(selection < 3) { score_list = selection; navigate(interface_page::score_delete_confirmation); }
      else if(selection == 3) information(8);
      else navigate(interface_page::high_score_menu);
      break;
    case interface_page::score_delete_confirmation: campaign.clear_scores(score_list); navigate(interface_page::score_delete); break;
    case interface_page::high_scores: navigate(interface_page::high_score_menu); break;
    case interface_page::menu_information: navigate(information_return); break;
    case interface_page::documentation:
      if(document_chapter == 0 && !numeric_input.empty()) {
        unsigned int const chapter{static_cast<unsigned int>(std::stoi(numeric_input))};
        if(chapter < 1 || chapter > 17) break;
        document_chapter = chapter;
      } else if(++document_chapter > 17) document_chapter = 0;
      numeric_input.clear(); document(assets.text("corncob.doc"), document_chapter); break;
    case interface_page::registration: navigate(interface_page::menu); break;
    case interface_page::notice: feedback.clear(); navigate(information_return); break;
    case interface_page::help: if(++help_index == help_screens.size()) navigate(return_page); break;
    case interface_page::quit_confirmation:
      if(selection == 0) pending_finish = true;
      else page = interface_page::flight;
      break;
    case interface_page::outcome:
      session_running = false;
      navigate(latest_result ? interface_page::sortie_summary : interface_page::menu);
      break;
    case interface_page::flight: break;
    }
  } catch(std::exception const &error) { notice(error.what()); }
}
void front_end::command(interface_command const command) {
  /// Text readers scroll; entry lists navigate; confirmation dialogs accept only their documented keys
  bool const confirmation{page == interface_page::pilot_delete || page == interface_page::pilot_resurrect || page == interface_page::score_delete_confirmation};
  if(confirmation && command != interface_command::accept) {
    navigate(page == interface_page::score_delete_confirmation ? interface_page::score_delete : interface_page::roster);
    return;
  }
  bool const reading{page == interface_page::menu_information || page == interface_page::documentation || page == interface_page::registration || page == interface_page::briefing || page == interface_page::airfield_information};
  unsigned int const visible_lines{page == interface_page::airfield_information ? 9u : page == interface_page::menu_information ? 15u : page == interface_page::briefing ? 16u : 22u};
  auto scroll_to{[&](unsigned int const line){
    unsigned int count{static_cast<unsigned int>(document_lines.size())};
    if(page == interface_page::briefing) {
      count = 0;
      std::istringstream text{briefing}; std::string line;
      while(std::getline(text, line)) if(line.find('\f') == std::string::npos && !line.starts_with("Sxyth")) ++count;
    }
    scroll = std::min(line, count > visible_lines ? count - visible_lines : 0u);
  }};
  switch(command) {
  case interface_command::accept: accept(); break;
  case interface_command::backspace:
    if(editing_name()) {
      auto &text{page == interface_page::training_value ? numeric_input : entered_name};
      if(!text.empty()) text.pop_back();
    } else if(page == interface_page::documentation && document_chapter == 0 && !numeric_input.empty()) numeric_input.pop_back();
    else this->command(interface_command::cancel);
    break;
  case interface_command::up:
  case interface_command::down:
    if(reading) scroll_to(command == interface_command::up ? (scroll == 0 ? 0 : scroll - 1) : scroll + 1);
    else if(page == interface_page::records || page == interface_page::medals) {
      unsigned int const count{static_cast<unsigned int>(campaign.pilots().size())};
      pilot_index = (pilot_index + (command == interface_command::up ? count - 1 : 1)) % count;
    } else {
      unsigned int const count{choice_count()};
      selection = (selection + (command == interface_command::up ? count - 1 : 1)) % count;
    }
    break;
  case interface_command::home: if(reading) scroll = 0; else selection = 0; break;
  case interface_command::end: if(reading) scroll_to(static_cast<unsigned int>(document_lines.size())); else selection = choice_count() - 1; break;
  case interface_command::page_down: if(reading) scroll_to(scroll + visible_lines - 1); break;
  case interface_command::page_up: if(reading) scroll_to(scroll >= visible_lines - 1 ? scroll - visible_lines + 1 : 0); break;
  case interface_command::details:
    if(page == interface_page::records || page == interface_page::medals) detailed_records = !detailed_records;
    break;
  case interface_command::previous:
    if(page == interface_page::help && help_index != 0) --help_index;
    break;
  case interface_command::next:
    if(page == interface_page::help || page == interface_page::story || page == interface_page::title) accept();
    break;
  case interface_command::cancel:
    switch(page) {
    case interface_page::radio: accept(); break;
    case interface_page::ending: ending.reset(); show_outcome(14); break;
    case interface_page::flight: ask_to_end_sortie(); break;
    case interface_page::help: navigate(return_page); break;
    case interface_page::menu: if(session_running) page = interface_page::flight; else quit_requested = true; break;
    case interface_page::mission_briefing: prepared_launch.reset(); navigate(interface_page::training); break;
    case interface_page::starting_airfield: prepared_launch.reset(); navigate(interface_page::theatres); break;
    case interface_page::airfield_information: navigate(interface_page::starting_airfield); break;
    case interface_page::pilots: case interface_page::pilot_name: case interface_page::pilot_delete:
    case interface_page::records: case interface_page::medals: case interface_page::pilot_resurrect: navigate(interface_page::roster); break;
    case interface_page::pilot_activate: selection = 1; accept(); break;
    case interface_page::new_theatre: case interface_page::open_theatre: case interface_page::theatre_delete:
    case interface_page::theatre_records: detailed_records = false; navigate(interface_page::theatres); break;
    case interface_page::training_counts: case interface_page::training_wickedness: navigate(interface_page::training); break;
    case interface_page::training_value: feedback.clear(); navigate(training_editor); break;
    case interface_page::score_delete: case interface_page::high_scores: navigate(interface_page::high_score_menu); break;
    case interface_page::score_delete_confirmation: navigate(interface_page::score_delete); break;
    case interface_page::menu_information: navigate(information_return); break;
    case interface_page::documentation:
      if(document_chapter != 0) { document_chapter = 0; document(assets.text("corncob.doc"), 0); }
      else navigate(interface_page::menu);
      break;
    case interface_page::notice: accept(); break;
    case interface_page::title: case interface_page::story: title.reset(); navigate(interface_page::menu); break;
    case interface_page::briefing: case interface_page::missions: case interface_page::current_score: case interface_page::boss:
    case interface_page::quit_confirmation: page = interface_page::flight; break;
    default: navigate(interface_page::menu); break;
    }
    break;
  case interface_command::help:
    return_page = page; help_index = 0; navigate(interface_page::help); break;
  }
}
void front_end::begin_ending(graphics::indexed_framebuffer const &backdrop, std::array<render::rgba_pixel, 16> const &colours) {
  /// Preserve the final sortie page while the single executable performs the original SRATS handoff
  ending.emplace(backdrop, colours);
  page = interface_page::ending;
}
void front_end::advance(double const seconds) {
  /// Helper animations continue on wall time while flight state remains stopped
  if(page == interface_page::story) {
    double constexpr palette_steps_per_second{40.0}, maximum_intensity{16.0};
    story_intensity = std::clamp(story_intensity + seconds * palette_steps_per_second * (advancing_story ? -1.0 : 1.0), 0.0, maximum_intensity);
    if(advancing_story && story_intensity == 0.0) {
      advancing_story = false;
      if(++story_index == stories.size()) { title.emplace(assets); page = interface_page::title; }
    }
  }
  if(page == interface_page::title) title->advance(seconds);
  if(page == interface_page::ending) {
    ending->advance(seconds);
    if(ending->finished()) { ending.reset(); show_outcome(14); }
  }
}
void front_end::show_radio(resources::radio_notice const notice, render::framebuffer const &backdrop, bool const enter_map) {
  /// BLACKBOX overlays the retained flight screen and pauses flight until the pilot acknowledges its original text
  radio = resources::read_radio_message(notice);
  radio_backdrop = backdrop;
  map_after_radio = enter_map;
  page = interface_page::radio;
}
void front_end::leave_sortie() {
  /// The flight session has already applied the original one-time Escape warning rule
  pending_finish = true;
}
void front_end::abort_sortie() {
  /// Control-C is a distinct original exit path with the abort result bit and no awarded score
  pending_abort = true;
  pending_finish = true;
}
bool front_end::take_abort() {
  /// Consume abort status alongside the single finalization request
  return std::exchange(pending_abort, false);
}
bool front_end::take_map() {
  /// Map instructions return to the original retained session before entering its independent observer loop
  return std::exchange(pending_map, false);
}
void front_end::show_menu() {
  /// The native helper flow can return to a retained flight without recreating its world
  page = interface_page::menu;
  selection = 0;
}
void front_end::begin_flight() {
  /// A newly created native session owns the selected theatre until its sortie ends
  session_running = true;
  page = interface_page::flight;
}
void front_end::show_flight_help() {
  /// F4 opens the original in-flight help sequence and returns to the same session
  session_running = true;
  return_page = interface_page::flight;
  page = interface_page::help;
  help_index = 0;
}
void front_end::ask_to_end_sortie() {
  /// Escape requires a deliberate confirmation and keeps the current flight available on cancellation
  session_running = true;
  page = interface_page::quit_confirmation;
  selection = 1;
}
void front_end::show_outcome(unsigned int const index) {
  /// Outcome selection comes from sortie state; rendering retains the original stored composition
  outcome_index = std::min(index, static_cast<unsigned int>(outcomes.size() - 1));
  page = interface_page::outcome;
}
void front_end::draw_menu(graphics::interface_canvas &canvas, std::string_view const title, std::span<std::string_view const> const choices) const {
  /// Retain a simple BIOS-font interface, with selection and instructions visible at the same time
  canvas.clear();
  canvas.rectangle(4, 20, 636, 392, blue);
  canvas.box(0, 16, 640, 396, yellow);
  canvas.box(2, 18, 638, 394, white);
  canvas.rectangle(168, 0, 464, 76, blue);
  canvas.box(172, 4, 460, 72, white);
  canvas.text(272, 12, "Corncob 3D", white);
  canvas.text(316 - static_cast<int>(title.size()) * 4, 44, title, grey);
  if(!choices.empty()) canvas.text(80, 92, page == interface_page::training || page == interface_page::control_setup
    ? "Left/Right changes values; Enter selects" : "Use arrow keys and Enter to choose an action:", {85, 255, 255});
  unsigned int const first{selection < 8 ? 0 : selection - 7};
  for(unsigned int n{first}; n < choices.size() && n < first + 8; ++n) {
    int const y{112 + static_cast<int>(n - first) * 24};
    if(n == selection) canvas.rectangle(96, y, 600, y + 16, {0, 0, 85});
    canvas.text(112, y, choices[n], n == selection ? yellow : grey);
  }
  if(!feedback.empty()) canvas.text(24, 316, feedback.substr(0, 74), yellow);
  canvas.text(72, 348, "Arrows select   Enter accepts   Esc returns", grey);
  canvas.text(72, 368, "F4: flight controls and help", grey);
}
void front_end::render(render::framebuffer &target) const {
  /// Menus and helper-program artwork share the native window but retain their original video-mode pixel sizes
  if(page == interface_page::radio) {
    target = radio_backdrop;
    graphics::interface_canvas canvas{target, assets.file("font14.bin"), 14};
    auto fill{[&](resources::message_box const &box, render::rgba_pixel const colour){canvas.rectangle(box.left, box.top, box.left + box.width, box.top + box.height, colour);}};
    fill(radio->border, {170, 0, 0});
    fill(radio->background, {0, 0, 0});
    for(auto const &line : radio->lines) canvas.text(line.column * 8, line.row * 14, line.text, grey);
    return;
  }
  if(page == interface_page::ending) { ending->render(target); return; }
  if(page == interface_page::boss) {
    graphics::interface_canvas canvas{target, assets.file("font16.bin"), 16, 640, 400};
    canvas.clear();
    auto text{std::string{assets.text("boss.txt")}};
    if(auto const end{text.find('$')}; end != std::string::npos) text.resize(end);
    canvas.text(0, 0, text, grey);
    return;
  }
  if(page == interface_page::story) {
    graphics::draw_presentation(target, assets, stories.at(story_index));
    unsigned int const intensity{static_cast<unsigned int>(story_intensity)};
    if(intensity < 16) {
      auto channel{[intensity](uint8_t const value){
        unsigned int const dac{(value >> 2) * intensity / 16};
        return static_cast<uint8_t>((dac << 2) | (dac >> 4));
      }};
      for(auto &pixel : target.pixels) pixel = {
        .red{channel(pixel.red)},
        .green{channel(pixel.green)},
        .blue{channel(pixel.blue)},
      };
    }
    return;
  }
  if(page == interface_page::outcome) {
    graphics::draw_presentation(target, assets, outcomes.at(outcome_index));
    return;
  }
  if(page == interface_page::help) {
    resources::draw_cockpit_preview(help_screens.at(help_index), target);
    target.width = 640;
    target.height = 350;
    return;
  }
  if(page == interface_page::title) {
    title->render(target);
    return;
  }
  render_menus(target);
}
unsigned int front_end::choice_count() const {
  /// Counts describe the original selectable entries, excluding status text and panel decorations
  switch(page) {
  case interface_page::menu: return session_running ? 3 : 11;
  case interface_page::roster: return 9;
  case interface_page::theatres: return 10;
  case interface_page::training: case interface_page::control_setup: return 7;
  case interface_page::training_counts: case interface_page::training_wickedness: return 4;
  case interface_page::new_theatre: return 4;
  case interface_page::open_theatre: case interface_page::theatre_delete: case interface_page::theatre_records:
    return std::max(1u, static_cast<unsigned int>(saved_theatres.size()));
  case interface_page::high_score_menu: return 6;
  case interface_page::score_delete: return 5;
  case interface_page::pilot_activate: case interface_page::quit_confirmation: return 2;
  case interface_page::missions: return static_cast<unsigned int>(mission_starts.size()) + 1;
  case interface_page::pilots: return std::max(1u, static_cast<unsigned int>(campaign.pilots().size()));
  default: return 1;
  }
}
audio::music_cue front_end::music() const noexcept {
  /// The DOS launcher plays music; flight, its helper pages and the original outcome/ending sequences do not
  if(!music_on || session_running || page == interface_page::ending || page == interface_page::outcome || page == interface_page::story)
    return audio::music_cue::silent;
  return page == interface_page::title ? audio::music_cue::title : audio::music_cue::menus;
}
control_setup const &front_end::flight_controls() const noexcept {
  /// New sessions and platform polling consume the same settings shown by the front end
  return controls;
}
void front_end::show_intelligence(world::theatre const &world) {
  /// The retained session supplies its current theatre's complete intelligence text
  session_running = true;
  briefing = world.briefing;
  scroll = 0;
  page = interface_page::briefing;
}
void front_end::show_missions(world::theatre const &world) {
  /// Intelligence starts are explicit directives in the original theatre text
  session_running = true;
  mission_starts = world.missions;
  briefing = world.briefing;
  selection = 0;
  page = interface_page::missions;
}
void front_end::show_boss() {
  /// F1 retains the original deliberately uninteresting DOS prompt disguise
  session_running = true;
  page = interface_page::boss;
}
void front_end::show_current_score(sortie_statistics const &result) {
  /// F3 inspects this sortie without finalizing or saving it
  latest_result = result;
  session_running = true;
  page = interface_page::current_score;
}
std::optional<world::mission_start> front_end::take_relocation() {
  /// Consume a mission selection without recreating the theatre or losing already destroyed objectives
  return std::exchange(pending_relocation, std::nullopt);
}
void front_end::character(uint32_t const codepoint) {
  /// Text prompts own characters; elsewhere the yellow original shortcut executes its menu entry immediately
  if(page == interface_page::pilot_name) {
    if(codepoint >= 32 && codepoint < 127 && entered_name.size() < 20) entered_name += static_cast<char>(codepoint);
    return;
  }
  if(codepoint == 32) return;
  if(page == interface_page::training_value || (page == interface_page::documentation && document_chapter == 0)) {
    if(codepoint >= '0' && codepoint <= '9' && numeric_input.size() < 2) numeric_input += static_cast<char>(codepoint);
    return;
  }
  if(page == interface_page::starting_airfield && (codepoint == 'i' || codepoint == 'I')) {
    document_lines = prepared_world ? resources::airfield_survey(*prepared_world) : std::vector<std::string>{};
    scroll = 0;
    navigate(interface_page::airfield_information);
    return;
  }
  if(page == interface_page::pilot_delete || page == interface_page::pilot_resurrect || page == interface_page::score_delete_confirmation) { command(interface_command::cancel); return; }
  if(page == interface_page::pilot_activate) {
    if(codepoint == 'y' || codepoint == 'Y' || codepoint == 'n' || codepoint == 'N') { selection = codepoint == 'n' || codepoint == 'N' ? 1 : 0; accept(); }
    return;
  }
  if(page == interface_page::high_scores || page == interface_page::notice) { accept(); return; }
  std::string_view shortcuts;
  switch(page) {
  case interface_page::menu: if(!session_running) shortcuts = "dcthrjmvpix"; break;
  case interface_page::roster: shortcuts = "cadlenvir"; break;
  case interface_page::theatres: shortcuts = "foadvpmcir"; break;
  case interface_page::training: shortcuts = "fwndpir"; break;
  case interface_page::training_counts: case interface_page::training_wickedness: shortcuts = "mgar"; break;
  case interface_page::control_setup: shortcuts = "kjamfir"; break;
  case interface_page::high_score_menu: shortcuts = "scfdir"; break;
  case interface_page::score_delete: shortcuts = "scfir"; break;
  case interface_page::story: case interface_page::title: accept(); return;
  default: break;
  }
  if(codepoint >= 128) return;
  auto const index{shortcuts.find(static_cast<char>(std::tolower(static_cast<unsigned char>(codepoint))))};
  if(index != std::string_view::npos) { selection = static_cast<unsigned int>(index); accept(); }
}
bool front_end::editing_name() const noexcept {
  /// Space and Backspace belong to the active text prompt, rather than triggering menu navigation
  return page == interface_page::pilot_name || page == interface_page::training_value;
}
bool front_end::take_finish() {
  /// Consume one confirmed end request; the application records and presents the actual result
  return std::exchange(pending_finish, false);
}
void front_end::finish_sortie(sortie_statistics const &result) {
  /// Match native result priority before showing the sortie statistics in the same executable
  latest_result = result;
  session_running = false;
  unsigned int panel{9};
  if(result.has(sortie_status::earth_saved)) panel = 14;
  else if(result.has(sortie_status::killed)) {
    panel = result.has(sortie_status::ejected) ? 7 : 4;
    if(result.has(sortie_status::ejected)) switch(static_cast<world::object_type>(result.contacting_type)) {
    case world::object_type::aa_shell: case world::object_type::shell: panel = 10; break;
    case world::object_type::kla: panel = 11; break;
    case world::object_type::crater: panel = 12; break;
    case world::object_type::bee_swarm: case world::object_type::barrage_balloon: panel = 13; break;
    default: break;
    }
  } else if(result.has(sortie_status::captured)) panel = 8;
  else if(result.has(sortie_status::home) && result.friendly_destroyed != 0) panel = 6;
  else if(result.has(sortie_status::rescued)) panel = 1;
  else if(result.has(sortie_status::crashed_away)) panel = 2;
  else if(result.has(sortie_status::crashed_home)) panel = 3;
  else if(result.has(sortie_status::home)) panel = result.has(sortie_status::landed) ? 0 : 5;
  show_outcome(panel);
}
std::optional<launch_request> front_end::take_launch() {
  /// Consume launch requests once so the main loop can replace its owned session safely
  return std::exchange(pending_launch, std::nullopt);
}
bool front_end::in_flight() const noexcept {
  /// Simulation and flight controls run only while the flight page owns input
  return page == interface_page::flight;
}
bool front_end::quitting() const noexcept {
  /// Keep window-system shutdown outside the core menu state machine
  return quit_requested;
}
interface_page front_end::current_page() const noexcept {
  /// Expose page state for scripted UI traversal without coupling to GLFW
  return page;
}

} // namespace corncob::game
